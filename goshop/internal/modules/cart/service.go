package cart

import (
	"goshop/internal/database"
	"goshop/internal/models"
)

// AddToCart 添加商品到购物车
func AddToCart(userID, productID uint, quantity int) error {
	var item models.CartItem
	result := database.DB.Where("user_id = ? AND product_id = ?", userID, productID).First(&item)

	if result.Error == nil {
		item.Quantity += quantity
		return database.DB.Save(&item).Error
	}

	newItem := models.CartItem{
		UserID:    userID,
		ProductID: productID,
		Quantity:  quantity,
	}
	return database.DB.Create(&newItem).Error
}

// UpdateQuantity 修改购物车商品数量
func UpdateQuantity(userID, itemID uint, quantity int) error {
	if quantity <= 0 {
		return database.DB.Where("id = ? AND user_id = ?", itemID, userID).Delete(&models.CartItem{}).Error
	}
	return database.DB.Model(&models.CartItem{}).
		Where("id = ? AND user_id = ?", itemID, userID).
		Update("quantity", quantity).Error
}

// Remove 移除购物车商品
func Remove(userID, itemID uint) error {
	return database.DB.Where("id = ? AND user_id = ?", itemID, userID).Delete(&models.CartItem{}).Error
}

// List 获取用户购物车列表
func List(userID uint) ([]models.CartItem, error) {
	var items []models.CartItem
	if err := database.DB.Preload("Product.Category").
		Where("user_id = ?", userID).
		Order("id DESC").
		Find(&items).Error; err != nil {
		return nil, err
	}
	return items, nil
}

// GetByIDs 根据ID列表获取购物车项
func GetByIDs(userID uint, itemIDs []uint) ([]models.CartItem, error) {
	var items []models.CartItem
	if err := database.DB.Preload("Product").
		Where("user_id = ? AND id IN ?", userID, itemIDs).
		Find(&items).Error; err != nil {
		return nil, err
	}
	return items, nil
}
