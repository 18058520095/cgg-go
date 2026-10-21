package middleware

import (
	"log"
	"runtime/debug"

	"goshop/internal/utils"

	"github.com/gin-gonic/gin"
)

// Recovery panic 恢复中间件
func Recovery() gin.HandlerFunc {
	return func(c *gin.Context) {
		defer func() {
			if err := recover(); err != nil {
				log.Printf("[panic] %v\n%s", err, debug.Stack())
				utils.FailWithStatus(c, 500, 500, "服务器内部错误")
				c.Abort()
			}
		}()
		c.Next()
	}
}
