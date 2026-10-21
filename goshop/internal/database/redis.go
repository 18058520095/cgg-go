package database

import (
	"context"
	"fmt"
	"log"
	"time"

	"goshop/internal/config"

	"github.com/redis/go-redis/v9"
)

var RDB *redis.Client

// InitRedis 初始化 Redis 连接
func InitRedis() {
	cfg := config.Cfg.Redis

	RDB = redis.NewClient(&redis.Options{
		Addr:     fmt.Sprintf("%s:%s", cfg.Host, cfg.Port),
		Password: cfg.Password,
		DB:       cfg.DB,
	})

	ctx, cancel := context.WithTimeout(context.Background(), 3*time.Second)
	defer cancel()

	if _, err := RDB.Ping(ctx).Result(); err != nil {
		log.Fatalf("[database] Redis 连接失败: %v", err)
	}

	log.Println("[database] Redis 连接成功")
}
