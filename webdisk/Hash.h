#ifndef CLOUDISK_HASH_H_
#define CLOUDISK_HASH_H_

#include <string>

// 哈希工具：基于 OpenSSL EVP 实现
class Hash
{
public:
    // 计算内容的 sha1，返回 40 位十六进制字符串（与 tbl_file.file_sha1 char(40) 对应）
    static std::string sha1(const std::string &content);
};

#endif
