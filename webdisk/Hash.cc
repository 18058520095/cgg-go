#include "Hash.h"

#include <openssl/evp.h>

#include <cstdio>

std::string Hash::sha1(const std::string &content)
{
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int md_len = 0;

    if (EVP_Digest(content.data(), content.size(), md, &md_len, EVP_sha1(), nullptr) != 1)
    {
        return std::string();
    }

    std::string result;
    result.reserve(md_len * 2);
    char buf[3];
    for (unsigned int i = 0; i < md_len; ++i)
    {
        snprintf(buf, sizeof(buf), "%02x", md[i]);
        result.append(buf);
    }
    return result;
}
