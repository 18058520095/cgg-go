package cart

import (
	"goshop/internal/utils"
	"strconv"

	"github.com/gin-gonic/gin"
)

// AddToCartRequest 添加购物车请求
type AddToCartRequest struct {
	ProductID uint `json:"product_id" binding:"required"`
	Quantity  int  `json:"quantity" binding:"required,min=1"`
}

// UpdateQuantityRequest 修改数量请求
type UpdateQuantityRequest struct {
	Quantity int `json:"quantity" binding:"required,min=1"`
}

// AddToCartHandler 添加到购物车
func AddToCartHandler(c *gin.Context) {
	userID := c.GetUint("user_id")

	var req AddToCartRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		utils.Fail(c, 400, "参数错误: "+err.Error())
		return
	}

	if err := AddToCart(userID, req.ProductID, req.Quantity); err != nil {
		utils.Fail(c, 500, "添加购物车失败")
		return
	}
	utils.SuccessMsg(c, "添加成功", nil)
}

// UpdateQuantityHandler 修改数量
func UpdateQuantityHandler(c *gin.Context) {
	userID := c.GetUint("user_id")
	itemID, err := strconv.Atoi(c.Param("id"))
	if err != nil {
		utils.Fail(c, 400, "无效的购物车项ID")
		return
	}

	var req UpdateQuantityRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		utils.Fail(c, 400, "参数错误: "+err.Error())
		return
	}

	if err := UpdateQuantity(userID, uint(itemID), req.Quantity); err != nil {
		utils.Fail(c, 500, "修改数量失败")
		return
	}
	utils.SuccessMsg(c, "修改成功", nil)
}

// RemoveHandler 移除购物车商品
func RemoveHandler(c *gin.Context) {
	userID := c.GetUint("user_id")
	itemID, err := strconv.Atoi(c.Param("id"))
	if err != nil {
		utils.Fail(c, 400, "无效的购物车项ID")
		return
	}

	if err := Remove(userID, uint(itemID)); err != nil {
		utils.Fail(c, 500, "移除失败")
		return
	}
	utils.SuccessMsg(c, "移除成功", nil)
}

// ListHandler 获取购物车列表
func ListHandler(c *gin.Context) {
	userID := c.GetUint("user_id")
	items, err := List(userID)
	if err != nil {
		utils.Fail(c, 500, "获取购物车列表失败")
		return
	}
	utils.Success(c, items)
}
