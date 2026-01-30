package main

import (
	"time"

	"github.com/gin-contrib/cors"
	"github.com/gin-gonic/gin"
)

func setupRouter() *gin.Engine {
	r := gin.Default()

	// Allow the Vite dev server (and localhost variants) to call this API.
	r.Use(cors.New(cors.Config{
		AllowAllOrigins: true,
		AllowMethods:    []string{"POST", "OPTIONS"},
		AllowHeaders:    []string{"Content-Type"},
		MaxAge:          12 * time.Hour,
	}))

	r.POST("/compute_dijkstra_path", handleCompute)
	r.POST("/compute_analytics_path", handleAnalytics)
	r.POST("/compute_heat_path", handleHeat)

	// health endpoint
	r.GET("/health", func(c *gin.Context) {
		c.JSON(200, gin.H{"status": "✅ ok"})
	})

	return r
}
