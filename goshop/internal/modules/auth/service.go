package auth

import (
	"goshop/internal/database"
	"goshop/internal/models"
	"goshop/internal/utils"

	"gorm.io/gorm"
)

// Register 注册
func Register(username, password, email string) (*models.User, error) {
	var count int64
	database.DB.Model(&models.User{}).Where("username = ? OR email = ?", username, email).Count(&count)
	if count > 0 {
		return nil, gorm.ErrRecordNotFound
	}

	hashedPwd, err := utils.HashPassword(password)
	if err != nil {
		return nil, err
	}

	user := models.User{
		Username: username,
		Password: hashedPwd,
		Email:    email,
		Role:     "user",
	}
	if err := database.DB.Create(&user).Error; err != nil {
		return nil, err
	}
	return &user, nil
}

// Login 登录
func Login(username, password string) (*models.User, string, error) {
	var user models.User
	if err := database.DB.Where("username = ?", username).First(&user).Error; err != nil {
		return nil, "", err
	}

	if !utils.CheckPassword(password, user.Password) {
		return nil, "", gorm.ErrRecordNotFound
	}

	token, err := utils.GenerateToken(user.ID, user.Username, user.Role)
	if err != nil {
		return nil, "", err
	}
	return &user, token, nil
}

// GetUserByID 根据ID获取用户
func GetUserByID(id uint) (*models.User, error) {
	var user models.User
	if err := database.DB.First(&user, id).Error; err != nil {
		return nil, err
	}
	return &user, nil
}
