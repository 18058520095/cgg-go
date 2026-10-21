#include "Token.h"

#include "Hash.h"

#include <chrono>
#include <random>

std::string Token::generate_token(const std::string &username)
{
    // 每次调用使用独立的随机源，避免并发请求相互影响
    std::random_device rd;
    unsigned long long seed =
        (static_cast<unsigned long long>(rd()) << 32) ^ rd() ^
        static_cast<unsigned long long>(
            std::chrono::system_clock::now().time_since_epoch().count());
    std::mt19937_64 gen(seed);

    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto stamp = std::chrono::duration_cast<std::chrono::microseconds>(now).count();

    std::string raw = username + ":" + std::to_string(stamp) + ":" + std::to_string(gen());
    return Hash::sha1(raw);
}
