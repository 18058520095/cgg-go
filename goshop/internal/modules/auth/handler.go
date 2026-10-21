package auth

import (
	"goshop/internal/utils"

	"github.com/gin-gonic/gin"
)

// RegisterRequest 注册请求
type RegisterRequest struct {
	Username string `json:"username" binding:"required,min=3,max=50"`
	Password string `json:"password" binding:"required,min=6,max=50"`
	Email    string `json:"email" binding:"omitempty,email"`
}

// LoginRequest 登录请求
type LoginRequest struct {
	Username string `json:"username" binding:"required"`
	Password string `json:"password" binding:"required"`
}

// RegisterHandler 注册接口
func RegisterHandler(c *gin.Context) {
	var req RegisterRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		utils.Fail(c, 400, "请求参数错误: "+err.Error())
		return
	}

	user, err := Register(req.Username, req.Password, req.Email)
	if err != nil {
		utils.Fail(c, 400, "注册失败：用户名或邮箱已存在")
		return
	}

	utils.Success(c, gin.H{
		"id":       user.ID,
		"username": user.Username,
		"email":    user.Email,
	})
}

// LoginHandler 登录接口
func LoginHandler(c *gin.Context) {
	var req LoginRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		utils.Fail(c, 400, "请求参数错误: "+err.Error())
		return
	}

	user, token, err := Login(req.Username, req.Password)
	if err != nil {
		utils.Fail(c, 401, "用户名或密码错误")
		return
	}

	utils.Success(c, gin.H{
		"token": token,
		"user": gin.H{
			"id":       user.ID,
			"username": user.Username,
			"email":    user.Email,
			"role":     user.Role,
		},
	})
}

// MeHandler 获取当前登录用户信息
func MeHandler(c *gin.Context) {
	userID := c.GetUint("user_id")
	user, err := GetUserByID(userID)
	if err != nil {
		utils.Fail(c, 404, "用户不存在")
		return
	}

	utils.Success(c, user)
}
