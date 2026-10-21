package main

import (
	"fmt"
	"log"

	"goshop/internal/config"
	"goshop/internal/database"
	"goshop/internal/router"

	"github.com/gin-gonic/gin"
)

func main() {
	config.Load()

	gin.SetMode(config.Cfg.App.Mode)

	database.Init()
	database.InitRedis()

	r := gin.New()
	router.Setup(r)

	addr := fmt.Sprintf(":%s", config.Cfg.App.Port)
	log.Printf("[server] 服务启动，监听地址: %s", addr)
	if err := r.Run(addr); err != nil {
		log.Fatalf("[server] 启动失败: %v", err)
	}
}
