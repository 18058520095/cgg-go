package models

import "time"

// Order 订单主表
type Order struct {
	ID            uint        `gorm:"primaryKey" json:"id"`
	OrderNo       string      `gorm:"size:50;uniqueIndex;not null" json:"order_no"`
	UserID        uint        `gorm:"index;not null" json:"user_id"`
	TotalAmount   float64     `gorm:"type:decimal(10,2);not null" json:"total_amount"`
	Status        int         `gorm:"default:1" json:"status"`
	PaymentStatus int         `gorm:"default:0" json:"payment_status"`
	ShippingName  string      `gorm:"size:50" json:"shipping_name"`
	ShippingPhone string      `gorm:"size:20" json:"shipping_phone"`
	ShippingAddr  string      `gorm:"size:255" json:"shipping_addr"`
	Remark        string      `gorm:"size:500" json:"remark"`
	CreatedAt     time.Time   `json:"created_at"`
	UpdatedAt     time.Time   `json:"updated_at"`

	Items []OrderItem `gorm:"foreignKey:OrderID" json:"items"`
	User  User        `gorm:"foreignKey:UserID" json:"-"`
}

func (Order) TableName() string { return "orders" }

// OrderItem 订单明细
type OrderItem struct {
	ID        uint    `gorm:"primaryKey" json:"id"`
	OrderID   uint    `gorm:"index;not null" json:"order_id"`
	ProductID uint    `gorm:"not null" json:"product_id"`
	Name      string  `gorm:"size:100;not null" json:"name"`
	Price     float64 `gorm:"type:decimal(10,2);not null" json:"price"`
	Quantity  int     `gorm:"not null" json:"quantity"`
	Image     string  `gorm:"size:500" json:"image"`
}

func (OrderItem) TableName() string { return "order_items" }
