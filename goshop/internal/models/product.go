package models

import "time"

// Category 商品分类
type Category struct {
	ID        uint      `gorm:"primaryKey" json:"id"`
	Name      string    `gorm:"size:50;not null" json:"name"`
	Icon      string    `gorm:"size:255" json:"icon"`
	ParentID  uint      `gorm:"default:0" json:"parent_id"`
	Sort      int       `gorm:"default:0" json:"sort"`
	CreatedAt time.Time `json:"created_at"`
	UpdatedAt time.Time `json:"updated_at"`
}

func (Category) TableName() string { return "categories" }

// Product 商品模型
type Product struct {
	ID          uint      `gorm:"primaryKey" json:"id"`
	Name        string    `gorm:"size:100;not null" json:"name"`
	Description string    `gorm:"type:text" json:"description"`
	Price       float64   `gorm:"type:decimal(10,2);not null" json:"price"`
	Stock       int       `gorm:"not null;default:0" json:"stock"`
	Image       string    `gorm:"size:500" json:"image"`
	CategoryID  uint      `gorm:"index;not null" json:"category_id"`
	Status      int       `gorm:"default:1" json:"status"`
	CreatedAt   time.Time `json:"created_at"`
	UpdatedAt   time.Time `json:"updated_at"`

	Category Category `gorm:"foreignKey:CategoryID" json:"category"`
}

func (Product) TableName() string { return "products" }
