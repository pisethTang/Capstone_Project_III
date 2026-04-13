package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"

	"github.com/gin-contrib/cors"
	"github.com/gin-gonic/gin"
)

type computeRequest struct {
	Start int    `json:"start"`
	End   int    `json:"end"`
	Model string `json:"model"`
}

type appDeps struct {
	resolveProjectRoot func() (string, error)
	runEngine          func(projectRoot, enginePath string, args ...string) ([]byte, error)
	readFile           func(path string) ([]byte, error)
}

func defaultResolveProjectRoot() (string, error) {
	if exe, err := os.Executable(); err == nil {
		exeDir := filepath.Dir(exe)
		return filepath.Clean(filepath.Join(exeDir, "..")), nil
	}
	wd, err := os.Getwd()
	if err != nil {
		return "", err
	}
	return filepath.Clean(filepath.Join(wd, "..")), nil
}

func defaultRunEngine(projectRoot, enginePath string, args ...string) ([]byte, error) {
	cmd := exec.Command(enginePath, args...)
	cmd.Dir = projectRoot
	return cmd.CombinedOutput()
}

func withDefaultDeps(deps appDeps) appDeps {
	if deps.resolveProjectRoot == nil {
		deps.resolveProjectRoot = defaultResolveProjectRoot
	}
	if deps.runEngine == nil {
		deps.runEngine = defaultRunEngine
	}
	if deps.readFile == nil {
		deps.readFile = os.ReadFile
	}
	return deps
}

func registerEngineRoute(r *gin.Engine, deps appDeps, routePath, resultFileName, mode string) {
	r.POST(routePath, func(c *gin.Context) {
		var req computeRequest
		if err := c.ShouldBindJSON(&req); err != nil {
			c.JSON(400, gin.H{"error": err.Error()})
			return
		}

		projectRoot, err := deps.resolveProjectRoot()
		if err != nil {
			c.JSON(500, gin.H{"error": "failed to determine project root"})
			return
		}

		// Only allow selecting a file name (prevents path traversal like ../../etc/passwd).
		modelName := filepath.Base(strings.ReplaceAll(req.Model, "\\", "/"))
		modelPath := filepath.Join(projectRoot, "frontend", "public", "data", modelName)
		enginePath := filepath.Join(projectRoot, "main")

		args := []string{fmt.Sprint(req.Start), fmt.Sprint(req.End), modelPath}
		if mode != "" {
			args = append(args, mode)
		}

		output, err := deps.runEngine(projectRoot, enginePath, args...)
		if err != nil {
			msg := strings.TrimSpace(string(output))
			if msg == "" {
				msg = err.Error()
			}
			c.JSON(500, gin.H{
				"error":     msg,
				"modelPath": modelPath,
			})
			return
		}

		resultPath := filepath.Join(projectRoot, "frontend", "public", resultFileName)
		payload, err := deps.readFile(resultPath)
		if err != nil {
			c.JSON(500, gin.H{"error": "failed to read " + resultFileName})
			return
		}

		c.Data(200, "application/json", payload)
	})
}

func buildRouter(deps appDeps) *gin.Engine {
	deps = withDefaultDeps(deps)

	r := gin.Default()

	// Allow the Vite dev server (and localhost variants) to call this API.
	r.Use(cors.New(cors.Config{
		AllowAllOrigins: true,
		AllowMethods:    []string{"POST", "OPTIONS"},
		AllowHeaders:    []string{"Content-Type"},
		MaxAge:          12 * time.Hour,
	}))

	registerEngineRoute(r, deps, "/compute", "result.json", "")
	registerEngineRoute(r, deps, "/analytics", "analytics.json", "analytics")
	registerEngineRoute(r, deps, "/heat", "heat_result.json", "heat")

	// health endpoint
	r.GET("/health", func(c *gin.Context) {
		c.JSON(200, gin.H{"status": "ok"})
	})

	return r
}

func main() {
	r := buildRouter(appDeps{})

	port := os.Getenv("PORT")
	if port == "" {
		port = "8080"
	}
	_ = r.Run(":" + port)
}
