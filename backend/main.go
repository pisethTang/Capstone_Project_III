package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"

	"github.com/gin-contrib/cors"
	"github.com/gin-gonic/gin" // Run: go get github.com/gin-gonic/gin
)

func main() {
	r := gin.Default()

	// Allow the Vite dev server (and localhost variants) to call this API.
	r.Use(cors.New(cors.Config{
		AllowAllOrigins: true,
		AllowMethods:    []string{"POST", "OPTIONS"},
		AllowHeaders:    []string{"Content-Type"},
		MaxAge:          12 * time.Hour,
	}))

	resolveProjectRoot := func() (string, error) {
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

	r.POST("/compute", func(c *gin.Context) {
		var req struct {
			Start int    `json:"start"`
			End   int    `json:"end"`
			Model string `json:"model"`
		}
		if err := c.ShouldBindJSON(&req); err != nil {
			c.JSON(400, gin.H{"error": err.Error()})
			return
		}

		projectRoot, err := resolveProjectRoot()
		if err != nil {
			c.JSON(500, gin.H{"error": "failed to determine project root"})
			return
		}

		// Only allow selecting a file name (prevents path traversal like ../../etc/passwd).
		modelName := filepath.Base(strings.ReplaceAll(req.Model, "\\", "/"))
		modelPath := filepath.Join(projectRoot, "frontend", "public", "data", modelName)
		enginePath := filepath.Join(projectRoot, "main")

		// Execute the C++ engine from the project root so its relative output path
		// (./frontend/public/result.json) lands in the right place.
		cmd := exec.Command(enginePath, fmt.Sprint(req.Start), fmt.Sprint(req.End), modelPath)
		cmd.Dir = projectRoot
		output, err := cmd.CombinedOutput()
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

		resultPath := filepath.Join(projectRoot, "frontend", "public", "result.json")
		payload, err := os.ReadFile(resultPath)
		if err != nil {
			c.JSON(500, gin.H{"error": "failed to read result.json"})
			return
		}

		c.Data(200, "application/json", payload)
	})

	r.POST("/analytics", func(c *gin.Context) {
		var req struct {
			Start int    `json:"start"`
			End   int    `json:"end"`
			Model string `json:"model"`
		}
		if err := c.ShouldBindJSON(&req); err != nil {
			c.JSON(400, gin.H{"error": err.Error()})
			return
		}

		projectRoot, err := resolveProjectRoot()
		if err != nil {
			c.JSON(500, gin.H{"error": "failed to determine project root"})
			return
		}

		modelName := filepath.Base(strings.ReplaceAll(req.Model, "\\", "/"))
		modelPath := filepath.Join(projectRoot, "frontend", "public", "data", modelName)
		enginePath := filepath.Join(projectRoot, "main")

		cmd := exec.Command(enginePath, fmt.Sprint(req.Start), fmt.Sprint(req.End), modelPath, "analytics")
		cmd.Dir = projectRoot
		output, err := cmd.CombinedOutput()
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

		resultPath := filepath.Join(projectRoot, "frontend", "public", "analytics.json")
		payload, err := os.ReadFile(resultPath)
		if err != nil {
			c.JSON(500, gin.H{"error": "failed to read analytics.json"})
			return
		}

		c.Data(200, "application/json", payload)
	})

	r.POST("/heat", func(c *gin.Context) {
		var req struct {
			Start int    `json:"start"`
			End   int    `json:"end"`
			Model string `json:"model"`
		}
		if err := c.ShouldBindJSON(&req); err != nil {
			c.JSON(400, gin.H{"error": err.Error()})
			return
		}

		projectRoot, err := resolveProjectRoot()
		if err != nil {
			c.JSON(500, gin.H{"error": "failed to determine project root"})
			return
		}

		modelName := filepath.Base(strings.ReplaceAll(req.Model, "\\", "/"))
		modelPath := filepath.Join(projectRoot, "frontend", "public", "data", modelName)
		enginePath := filepath.Join(projectRoot, "main")

		cmd := exec.Command(enginePath, fmt.Sprint(req.Start), fmt.Sprint(req.End), modelPath, "heat")
		cmd.Dir = projectRoot
		output, err := cmd.CombinedOutput()
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

		resultPath := filepath.Join(projectRoot, "frontend", "public", "heat_result.json")
		payload, err := os.ReadFile(resultPath)
		if err != nil {
			c.JSON(500, gin.H{"error": "failed to read heat_result.json"})
			return
		}

		c.Data(200, "application/json", payload)
	})

	port := os.Getenv("PORT")
	if port == "" {
		port = "8080"
	}
	r.Run(":" + port)
}
