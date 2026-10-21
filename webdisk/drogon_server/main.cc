// Cloudisk 网盘服务（Drogon 版本）
// 接口契约与原 wfrest 版本一致，前端 frontend/dist 可直接复用
#include <drogon/drogon.h>

int main()
{
    auto &app = drogon::app();

    // SPA 前端：/assets/* 等静态文件由 documentRoot 提供
    app.setDocumentRoot("./frontend/dist");

    // 上传目录与文件落地目录保持为同一个 ./tmp，避免 Drogon 默认创建 ./uploads
    app.setUploadPath("./tmp");

    // MySQL 连接池（utf8mb4：中文用户名/文件名正确存取）
    app.createDbClient("mysql", "127.0.0.1", 3306, "cloudisk", "root", "123",
                       2, "", "default", false, "utf8mb4");

    // Redis 连接池：登录态缓存 + 文件列表缓存
    app.createRedisClient("127.0.0.1", 6379);

    app.addListener("0.0.0.0", 1234);
    LOG_INFO << "cloudisk drogon server listening on 1234";
    app.run();
    return 0;
}
