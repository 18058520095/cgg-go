package order

import (
	"fmt"
	"time"

	"goshop/internal/database"
	"goshop/internal/models"
	"goshop/internal/modules/cart"
	"goshop/internal/modules/product"

	"gorm.io/gorm"
)

// CreateOrderRequest 创建订单请求
type CreateOrderRequest struct {
	CartItemIDs   []uint `json:"cart_item_ids" binding:"required"`
	ShippingName  string `json:"shipping_name" binding:"required"`
	ShippingPhone string `json:"shipping_phone" binding:"required"`
	ShippingAddr  string `json:"shipping_addr" binding:"required"`
	Remark        string `json:"remark"`
}

// Create 创建订单（事务：校验库存 -> 扣减库存 -> 创建订单 -> 清空购物车）
func Create(userID uint, req CreateOrderRequest) (*models.Order, error) {
	cartItems, err := cart.GetByIDs(userID, req.CartItemIDs)
	if err != nil {
		return nil, err
	}
	if len(cartItems) == 0 {
		return nil, fmt.Errorf("购物车项不存在")
	}

	var order *models.Order
	err = database.DB.Transaction(func(tx *gorm.DB) error {
		var totalAmount float64
		orderItems := make([]models.OrderItem, 0, len(cartItems))

		for _, ci := range cartItems {
			var p models.Product
			if err := tx.First(&p, ci.ProductID).Error; err != nil {
				return fmt.Errorf("商品不存在")
			}
			if p.Stock < ci.Quantity {
				return fmt.Errorf("商品《%s》库存不足", p.Name)
			}

			if err := product.DeductStock(tx, ci.ProductID, ci.Quantity); err != nil {
				return fmt.Errorf("扣减库存失败")
			}

			totalAmount += p.Price * float64(ci.Quantity)
			orderItems = append(orderItems, models.OrderItem{
				ProductID: p.ID,
				Name:      p.Name,
				Price:     p.Price,
				Quantity:  ci.Quantity,
				Image:     p.Image,
			})
		}

		order = &models.Order{
			OrderNo:       generateOrderNo(userID),
			UserID:        userID,
			TotalAmount:   totalAmount,
			Status:        1,
			PaymentStatus: 0,
			ShippingName:  req.ShippingName,
			ShippingPhone: req.ShippingPhone,
			ShippingAddr:  req.ShippingAddr,
			Remark:        req.Remark,
			Items:         orderItems,
		}
		if err := tx.Create(order).Error; err != nil {
			return err
		}

		if err := tx.Where("user_id = ? AND id IN ?", userID, req.CartItemIDs).Delete(&models.CartItem{}).Error; err != nil {
			return err
		}

		return nil
	})

	if err != nil {
		return nil, err
	}
	return order, nil
}

// GetByID 获取订单详情
func GetByID(orderID, userID uint) (*models.Order, error) {
	var o models.Order
	if err := database.DB.Preload("Items").
		Where("id = ? AND user_id = ?", orderID, userID).
		First(&o).Error; err != nil {
		return nil, err
	}
	return &o, nil
}

// List 用户订单列表
func List(userID uint, page, pageSize int) ([]models.Order, int64, error) {
	var orders []models.Order
	var total int64

	query := database.DB.Model(&models.Order{}).Where("user_id = ?", userID)
	if err := query.Count(&total).Error; err != nil {
		return nil, 0, err
	}

	offset := (page - 1) * pageSize
	if err := query.Preload("Items").
		Order("id DESC").
		Offset(offset).Limit(pageSize).
		Find(&orders).Error; err != nil {
		return nil, 0, err
	}
	return orders, total, nil
}

// Cancel 取消订单（恢复库存）
func Cancel(orderID, userID uint) error {
	var o models.Order
	if err := database.DB.Where("id = ? AND user_id = ?", orderID, userID).First(&o).Error; err != nil {
		return err
	}
	if o.Status != 1 {
		return fmt.Errorf("当前订单状态不允许取消")
	}

	return database.DB.Transaction(func(tx *gorm.DB) error {
		var items []models.OrderItem
		if err := tx.Where("order_id = ?", orderID).Find(&items).Error; err != nil {
			return err
		}
		for _, item := range items {
			if err := tx.Model(&models.Product{}).
				Where("id = ?", item.ProductID).
				Update("stock", gorm.Expr("stock + ?", item.Quantity)).Error; err != nil {
				return err
			}
		}

		return tx.Model(&o).Update("status", 5).Error
	})
}

func generateOrderNo(userID uint) string {
	return fmt.Sprintf("%d%d%d", userID, time.Now().UnixNano(), time.Now().UnixMicro()%1000)
}
