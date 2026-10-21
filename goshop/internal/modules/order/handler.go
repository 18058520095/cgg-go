package order

import (
	"goshop/internal/utils"
	"strconv"

	"github.com/gin-gonic/gin"
)

// CreateOrderHandler 创建订单
func CreateOrderHandler(c *gin.Context) {
	userID := c.GetUint("user_id")

	var req CreateOrderRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		utils.Fail(c, 400, "参数错误: "+err.Error())
		return
	}

	o, err := Create(userID, req)
	if err != nil {
		utils.Fail(c, 400, err.Error())
		return
	}
	utils.Success(c, o)
}

// GetOrderHandler 获取订单详情
func GetOrderHandler(c *gin.Context) {
	userID := c.GetUint("user_id")
	orderID, err := strconv.Atoi(c.Param("id"))
	if err != nil {
		utils.Fail(c, 400, "无效的订单ID")
		return
	}

	o, err := GetByID(uint(orderID), userID)
	if err != nil {
		utils.Fail(c, 404, "订单不存在")
		return
	}
	utils.Success(c, o)
}

// ListOrdersHandler 订单列表
func ListOrdersHandler(c *gin.Context) {
	userID := c.GetUint("user_id")
	page, _ := strconv.Atoi(c.DefaultQuery("page", "1"))
	pageSize, _ := strconv.Atoi(c.DefaultQuery("page_size", "10"))

	if page < 1 {
		page = 1
	}
	if pageSize < 1 || pageSize > 100 {
		pageSize = 10
	}

	orders, total, err := List(userID, page, pageSize)
	if err != nil {
		utils.Fail(c, 500, "获取订单列表失败")
		return
	}
	utils.Paginated(c, orders, total, page, pageSize)
}

// CancelOrderHandler 取消订单
func CancelOrderHandler(c *gin.Context) {
	userID := c.GetUint("user_id")
	orderID, err := strconv.Atoi(c.Param("id"))
	if err != nil {
		utils.Fail(c, 400, "无效的订单ID")
		return
	}

	if err := Cancel(uint(orderID), userID); err != nil {
		utils.Fail(c, 400, err.Error())
		return
	}
	utils.SuccessMsg(c, "订单已取消", nil)
}
