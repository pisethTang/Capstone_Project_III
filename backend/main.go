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
		AllowOrigins: []string{
			"http://localhost:5173",
			"http://127.0.0.1:5173",
			"http://localhost:3000",
			"http://127.0.0.1:3000",
		},
		AllowMethods: []string{"POST", "OPTIONS"},
		AllowHeaders: []string{"Content-Type"},
		MaxAge:       12 * time.Hour,
	}))

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

		// Resolve project root (assumes backend server is started from backend/).
		wd, err := os.Getwd()
		if err != nil {
			c.JSON(500, gin.H{"error": "failed to determine working directory"})
			return
		}
		projectRoot := filepath.Clean(filepath.Join(wd, ".."))

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

		c.JSON(200, gin.H{"status": "success"})
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

		wd, err := os.Getwd()
		if err != nil {
			c.JSON(500, gin.H{"error": "failed to determine working directory"})
			return
		}
		projectRoot := filepath.Clean(filepath.Join(wd, ".."))

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

		c.JSON(200, gin.H{"status": "success"})
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

		wd, err := os.Getwd()
		if err != nil {
			c.JSON(500, gin.H{"error": "failed to determine working directory"})
			return
		}
		projectRoot := filepath.Clean(filepath.Join(wd, ".."))

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

		c.JSON(200, gin.H{"status": "success"})
	})

	r.Run(":8080")
}
