package product

import (
	"goshop/internal/database"
	"goshop/internal/models"

	"gorm.io/gorm"
)

// Create 创建商品
func Create(p *models.Product) error {
	return database.DB.Create(p).Error
}

// Update 更新商品
func Update(p *models.Product) error {
	return database.DB.Save(p).Error
}

// Delete 删除商品
func Delete(id uint) error {
	return database.DB.Delete(&models.Product{}, id).Error
}

// GetByID 根据ID获取商品
func GetByID(id uint) (*models.Product, error) {
	var p models.Product
	if err := database.DB.Preload("Category").First(&p, id).Error; err != nil {
		return nil, err
	}
	return &p, nil
}

// List 商品列表（分页 + 关键词 + 分类筛选）
func List(keyword string, categoryID uint, page, pageSize int) ([]models.Product, int64, error) {
	var list []models.Product
	var total int64

	query := database.DB.Model(&models.Product{})
	if keyword != "" {
		query = query.Where("name LIKE ? OR description LIKE ?", "%"+keyword+"%", "%"+keyword+"%")
	}
	if categoryID > 0 {
		query = query.Where("category_id = ?", categoryID)
	}

	if err := query.Count(&total).Error; err != nil {
		return nil, 0, err
	}

	offset := (page - 1) * pageSize
	if err := query.Preload("Category").Order("id DESC").Offset(offset).Limit(pageSize).Find(&list).Error; err != nil {
		return nil, 0, err
	}
	return list, total, nil
}

// DeductStock 扣减库存（事务中调用）
func DeductStock(tx *gorm.DB, productID uint, quantity int) error {
	result := tx.Model(&models.Product{}).
		Where("id = ? AND stock >= ?", productID, quantity).
		Update("stock", gorm.Expr("stock - ?", quantity))
	if result.Error != nil {
		return result.Error
	}
	if result.RowsAffected == 0 {
		return gorm.ErrRecordNotFound
	}
	return nil
}
