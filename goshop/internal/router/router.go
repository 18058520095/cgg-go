package router

import (
	"net/http"

	"goshop/internal/middleware"
	"goshop/internal/modules/auth"
	"goshop/internal/modules/cart"
	"goshop/internal/modules/order"
	"goshop/internal/modules/product"
	"goshop/internal/webui"

	"github.com/gin-gonic/gin"
)

// Setup 注册所有路由
func Setup(r *gin.Engine) {
	r.Use(middleware.CORS())
	r.Use(middleware.Recovery())
	r.Use(middleware.Logger())

	// 健康检查
	r.GET("/health", func(c *gin.Context) {
		c.JSON(200, gin.H{"status": "ok"})
	})

	// 前端单页（内嵌，无需额外静态文件）
	r.GET("/", func(c *gin.Context) {
		c.Data(http.StatusOK, "text/html; charset=utf-8", webui.IndexHTML)
	})

	api := r.Group("/api/v1")
	{
		// 公开路由
		api.POST("/auth/register", auth.RegisterHandler)
		api.POST("/auth/login", auth.LoginHandler)

		// 商品（公开可浏览）
		api.GET("/products", product.ListProductsHandler)
		api.GET("/products/:id", product.GetProductHandler)

		// 需要认证的路由
		authed := api.Group("")
		authed.Use(middleware.JWTAuth())
		{
			authed.GET("/auth/me", auth.MeHandler)

			authed.GET("/cart", cart.ListHandler)
			authed.POST("/cart", cart.AddToCartHandler)
			authed.PUT("/cart/:id", cart.UpdateQuantityHandler)
			authed.DELETE("/cart/:id", cart.RemoveHandler)

			authed.POST("/orders", order.CreateOrderHandler)
			authed.GET("/orders", order.ListOrdersHandler)
			authed.GET("/orders/:id", order.GetOrderHandler)
			authed.POST("/orders/:id/cancel", order.CancelOrderHandler)
		}

		// 管理员路由
		admin := api.Group("/admin")
		admin.Use(middleware.JWTAuth(), middleware.AdminOnly())
		{
			admin.POST("/products", product.CreateProductHandler)
			admin.PUT("/products/:id", product.UpdateProductHandler)
			admin.DELETE("/products/:id", product.DeleteProductHandler)
		}
	}
}
