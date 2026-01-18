package main

import (
	"fmt"
	"os/exec"

	"github.com/gin-gonic/gin" // Run: go get github.com/gin-gonic/gin
)

func main() {
	r := gin.Default()

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

		// Execute the C++ Engine
		cmd := exec.Command("../main", fmt.Sprint(req.Start), fmt.Sprint(req.End), "../frontend/public/"+req.Model)
		output, err := cmd.CombinedOutput()
		if err != nil {
			c.JSON(500, gin.H{"error": string(output)})
			return
		}

		c.JSON(200, gin.H{"status": "success"})
	})

	r.Run(":8080")
}
