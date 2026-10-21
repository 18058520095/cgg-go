package product

import (
	"goshop/internal/models"
	"goshop/internal/utils"
	"strconv"

	"github.com/gin-gonic/gin"
)

// CreateProductRequest 创建商品请求
type CreateProductRequest struct {
	Name        string  `json:"name" binding:"required,max=100"`
	Description string  `json:"description"`
	Price       float64 `json:"price" binding:"required,gt=0"`
	Stock       int     `json:"stock" binding:"required,min=0"`
	Image       string  `json:"image"`
	CategoryID  uint    `json:"category_id" binding:"required"`
}

// CreateProductHandler 创建商品（管理员）
func CreateProductHandler(c *gin.Context) {
	var req CreateProductRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		utils.Fail(c, 400, "参数错误: "+err.Error())
		return
	}

	p := models.Product{
		Name:        req.Name,
		Description: req.Description,
		Price:       req.Price,
		Stock:       req.Stock,
		Image:       req.Image,
		CategoryID:  req.CategoryID,
		Status:      1,
	}
	if err := Create(&p); err != nil {
		utils.Fail(c, 500, "创建商品失败")
		return
	}
	utils.Success(c, p)
}

// UpdateProductHandler 更新商品（管理员）
func UpdateProductHandler(c *gin.Context) {
	id, err := strconv.Atoi(c.Param("id"))
	if err != nil {
		utils.Fail(c, 400, "无效的商品ID")
		return
	}

	var req CreateProductRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		utils.Fail(c, 400, "参数错误: "+err.Error())
		return
	}

	p, err := GetByID(uint(id))
	if err != nil {
		utils.Fail(c, 404, "商品不存在")
		return
	}

	p.Name = req.Name
	p.Description = req.Description
	p.Price = req.Price
	p.Stock = req.Stock
	p.Image = req.Image
	p.CategoryID = req.CategoryID

	if err := Update(p); err != nil {
		utils.Fail(c, 500, "更新商品失败")
		return
	}
	utils.Success(c, p)
}

// DeleteProductHandler 删除商品（管理员）
func DeleteProductHandler(c *gin.Context) {
	id, err := strconv.Atoi(c.Param("id"))
	if err != nil {
		utils.Fail(c, 400, "无效的商品ID")
		return
	}

	if err := Delete(uint(id)); err != nil {
		utils.Fail(c, 500, "删除商品失败")
		return
	}
	utils.SuccessMsg(c, "删除成功", nil)
}

// GetProductHandler 获取商品详情
func GetProductHandler(c *gin.Context) {
	id, err := strconv.Atoi(c.Param("id"))
	if err != nil {
		utils.Fail(c, 400, "无效的商品ID")
		return
	}

	p, err := GetByID(uint(id))
	if err != nil {
		utils.Fail(c, 404, "商品不存在")
		return
	}
	utils.Success(c, p)
}

// ListProductsHandler 商品列表
func ListProductsHandler(c *gin.Context) {
	keyword := c.Query("keyword")
	categoryID, _ := strconv.Atoi(c.Query("category_id"))
	page, _ := strconv.Atoi(c.DefaultQuery("page", "1"))
	pageSize, _ := strconv.Atoi(c.DefaultQuery("page_size", "10"))

	if page < 1 {
		page = 1
	}
	if pageSize < 1 || pageSize > 100 {
		pageSize = 10
	}

	list, total, err := List(keyword, uint(categoryID), page, pageSize)
	if err != nil {
		utils.Fail(c, 500, "查询商品列表失败")
		return
	}
	utils.Paginated(c, list, total, page, pageSize)
}
