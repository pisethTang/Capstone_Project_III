package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"

	"github.com/gin-gonic/gin"
)

type computeRequest struct {
	Start int    `json:"start"`
	End   int    `json:"end"`
	Model string `json:"model"`
}

func runEngine(req computeRequest, mode string, resultFilename string) ([]byte, string, error) {
	projectRoot, err := resolveProjectRoot()
	if err != nil {
		return nil, "", fmt.Errorf("failed to determine project root")
	}

	// Only allow selecting a file name (prevents path traversal like ../../etc/passwd).
	modelName := filepath.Base(strings.ReplaceAll(req.Model, "\\", "/"))
	modelPath := filepath.Join(projectRoot, "frontend", "public", "data", modelName)
	enginePath := filepath.Join(projectRoot, "engine", "bin", "main")

	args := []string{fmt.Sprint(req.Start), fmt.Sprint(req.End), modelPath}
	if mode != "" {
		args = append(args, mode)
	}

	// Execute the C++ engine from the project root so its relative output path
	// (./frontend/public/*.json) lands in the right place.
	cmd := exec.Command(enginePath, args...)
	cmd.Dir = projectRoot
	output, err := cmd.CombinedOutput()
	if err != nil {
		msg := strings.TrimSpace(string(output))
		if msg == "" {
			msg = err.Error()
		}
		return nil, modelPath, fmt.Errorf("%s", msg)
	}

	resultPath := filepath.Join(projectRoot, "frontend", "public", resultFilename)
	payload, err := os.ReadFile(resultPath)
	if err != nil {
		return nil, modelPath, fmt.Errorf("failed to read %s", resultFilename)
	}

	return payload, modelPath, nil
}

func handleCompute(c *gin.Context) {
	var req computeRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(400, gin.H{"error": err.Error()})
		return
	}

	payload, modelPath, err := runEngine(req, "", "result.json")
	if err != nil {
		c.JSON(500, gin.H{
			"error":     err.Error(),
			"modelPath": modelPath,
		})
		return
	}

	c.Data(200, "application/json", payload)
}

func handleAnalytics(c *gin.Context) {
	var req computeRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(400, gin.H{"error": err.Error()})
		return
	}

	payload, modelPath, err := runEngine(req, "analytics", "analytics.json")
	if err != nil {
		c.JSON(500, gin.H{
			"error":     err.Error(),
			"modelPath": modelPath,
		})
		return
	}

	c.Data(200, "application/json", payload)
}

func handleHeat(c *gin.Context) {
	var req computeRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(400, gin.H{"error": err.Error()})
		return
	}

	payload, modelPath, err := runEngine(req, "heat", "heat_result.json")
	if err != nil {
		c.JSON(500, gin.H{
			"error":     err.Error(),
			"modelPath": modelPath,
		})
		return
	}

	c.Data(200, "application/json", payload)
}
