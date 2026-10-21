#ifndef CLOUDISK_TOKEN_H_
#define CLOUDISK_TOKEN_H_

#include <string>

// 登录令牌工具：令牌长度为 40（与 tbl_user_token.user_token char(40) 对应）
class Token
{
public:
    // 由 用户名 + 时间戳 + 随机数 生成 token
    static std::string generate_token(const std::string &username);
};

#endif
