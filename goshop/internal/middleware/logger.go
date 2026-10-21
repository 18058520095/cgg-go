package middleware

import (
	"log"
	"time"

	"github.com/gin-gonic/gin"
)

// Logger 请求日志中间件
func Logger() gin.HandlerFunc {
	return func(c *gin.Context) {
		start := time.Now()
		c.Next()
		log.Printf("[%s] %s %d %v | %s",
			c.Request.Method, c.Request.URL.Path,
			c.Writer.Status(), time.Since(start), c.ClientIP())
	}
}
