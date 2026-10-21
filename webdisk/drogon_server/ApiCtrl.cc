// Cloudisk 业务接口
//   POST /user/signup        urlencoded username/password  -> "SUCCESS" | "Singup Failed"
//   POST /user/signin|sigin  urlencoded username/password  -> {"data":{Token,Username,Location}} | "Login Failed"
//   GET  /user/info          ?username=&token=             -> {"data":{Username,SignupAt}} | "error"
//   POST /file/query         ?username=&token= body limit  -> [{FileHash,FileName,...}] | "error"
//   POST /file/upload        ?username=&token= multipart   -> "upload Success" | "Singup Failed"
//   GET  /file/downloadurl   ?filename=                    -> nginx 下载地址 | "error"
// Redis：
//   cloudisk:token:<user>                登录态，SETEX 3600
//   cloudisk:flver:<user>                文件列表缓存版本号，上传成功后 INCR
//   cloudisk:filelist:<user>:<ver>:<lim> 文件列表缓存，SETEX 60
// 缓存只做加速：Redis 异常时全部回退 MySQL，不影响业务结果
#include "ApiCtrl.h"

#include <json/json.h>

#include <fstream>
#include <string>
#include <vector>

#include "../Hash.h"
#include "../Token.h"

using namespace drogon;
using Callback = std::function<void(const HttpResponsePtr &)>;
using CallbackPtr = std::shared_ptr<Callback>;

namespace
{
const std::string FILES_DIR = "./tmp/";
const std::string NGINX_URL = "http://127.0.0.1:8080";
const std::string FRONTEND_INDEX = "./frontend/dist/index.html";
const int TOKEN_CACHE_TTL = 3600;
const int FILELIST_CACHE_TTL = 60;

// ---- Redis key 约定 ----
std::string tokenKey(const std::string &username)
{
    return "cloudisk:token:" + username;
}

std::string flverKey(const std::string &username)
{
    return "cloudisk:flver:" + username;
}

std::string filelistKey(const std::string &username, const std::string &ver,
                        const std::string &limit)
{
    return "cloudisk:filelist:" + username + ":" + ver + ":" + limit;
}

orm::DbClientPtr db()
{
    return app().getDbClient();
}

nosql::RedisClientPtr redisCli()
{
    return app().getRedisClient();
}

HttpResponsePtr textResp(const std::string &body)
{
    auto resp = HttpResponse::newHttpResponse();
    resp->setBody(body);
    resp->setContentTypeCode(CT_TEXT_PLAIN);
    return resp;
}

// limit 参数必须是纯数字：它要直接拼进 SQL 的 LIMIT 子句（占位符不适用）
bool isNumberString(const std::string &str)
{
    if (str.empty())
        return false;
    for (char ch : str)
        if (ch < '0' || ch > '9')
            return false;
    return true;
}

// 取 Redis 字符串结果；键不存在或类型不符返回 false
bool redisString(const nosql::RedisResult &r, std::string &out)
{
    if (r.type() != nosql::RedisResultType::kString)
        return false;
    out = r.asString();
    return true;
}

// 执行一条 Redis 写命令。
// 注意：execCommandAsync 内部走 hiredis 的 printf 风格变参，
// %s 必须传 .c_str()，直接传 std::string 是未定义行为。
void redisWrite(const std::string &command, const std::vector<std::string> &params)
{
    auto onResult = [](const nosql::RedisResult &) {};
    auto onError = [](const std::exception &) {};
    try
    {
        switch (params.size())
        {
        case 1:
            redisCli()->execCommandAsync(onResult, onError, command + " %s",
                                         params[0].c_str());
            break;
        case 2:
            redisCli()->execCommandAsync(onResult, onError, command + " %s %s",
                                         params[0].c_str(), params[1].c_str());
            break;
        case 3:
            redisCli()->execCommandAsync(onResult, onError, command + " %s %s %s",
                                         params[0].c_str(), params[1].c_str(),
                                         params[2].c_str());
            break;
        default:
            break;
        }
    }
    catch (...)
    {
    }
}

void setTokenCache(const std::string &username, const std::string &token)
{
    redisWrite("SETEX", {tokenKey(username), std::to_string(TOKEN_CACHE_TTL), token});
}

// 回退 MySQL 校验登录态；refill 为 true 时把结果回填缓存
void checkTokenByDb(const std::string &username, const std::string &token,
                    const std::string &failMsg, CallbackPtr cb,
                    std::function<void()> onSuccess, bool refill)
{
    db()->execSqlAsync(
        "SELECT user_name FROM tbl_user_token WHERE user_name = ? AND user_token = ?",
        [=](const orm::Result &result) {
            if (result.empty())
            {
                (*cb)(textResp(failMsg));
                return;
            }
            if (refill)
                setTokenCache(username, token);
            onSuccess();
        },
        [=](const orm::DrogonDbException &) { (*cb)(textResp(failMsg)); },
        username, token);
}

// 登录态校验：Redis 优先，未命中（或 Redis 异常）回退 MySQL
void checkToken(const std::string &username, const std::string &token,
                const std::string &failMsg, CallbackPtr cb,
                std::function<void()> onSuccess)
{
    redisCli()->execCommandAsync(
        [=](const nosql::RedisResult &result) {
            std::string cached;
            if (redisString(result, cached))
            {
                // 缓存命中：直接比对，不再查库
                if (cached == token)
                    onSuccess();
                else
                    (*cb)(textResp(failMsg));  // 已被重新登录顶掉
                return;
            }
            checkTokenByDb(username, token, failMsg, cb, onSuccess, true);
        },
        [=](const std::exception &) {
            // Redis 不可用：只走 MySQL，不回填
            checkTokenByDb(username, token, failMsg, cb, onSuccess, false);
        },
        "GET %s", tokenKey(username).c_str());
}

// 查库取文件列表，写入缓存后响应
void queryFileListFromDb(const std::string &username, const std::string &ver,
                         const std::string &limit, CallbackPtr cb)
{
    // limit 已校验为纯数字，可直接拼接
    std::string sql =
        "SELECT file_sha1, file_name, file_size, upload_at, last_update "
        "FROM tbl_user_file WHERE user_name = ? AND status = 0 "
        "ORDER BY upload_at DESC LIMIT " + limit;

    db()->execSqlAsync(
        sql,
        [=](const orm::Result &result) {
            Json::Value files(Json::arrayValue);
            for (const auto &row : result)
            {
                Json::Value item;
                item["FileHash"] = row["file_sha1"].as<std::string>();
                item["FileName"] = row["file_name"].as<std::string>();
                item["FileSize"] = (Json::UInt64)row["file_size"].as<unsigned long long>();
                item["UploadAt"] = row["upload_at"].as<std::string>();
                item["LastUpdated"] = row["last_update"].as<std::string>();
                files.append(item);
            }

            std::string body = files.toStyledString();
            // 回填列表缓存：写缓存失败不影响本次响应
            redisWrite("SETEX", {filelistKey(username, ver, limit),
                                 std::to_string(FILELIST_CACHE_TTL), body});
            auto resp = HttpResponse::newHttpResponse();
            resp->setBody(body);
            resp->setContentTypeCode(CT_APPLICATION_JSON);
            (*cb)(resp);
        },
        [=](const orm::DrogonDbException &) { (*cb)(textResp("error")); },
        username);
}

HttpResponsePtr spaIndex()
{
    return HttpResponse::newFileResponse(FRONTEND_INDEX, "", CT_TEXT_HTML, "");
}
}  // namespace

// ---------------- 用户系统 ----------------

void ApiCtrl::signup(const HttpRequestPtr &req, Callback &&callback)
{
    CallbackPtr cb = std::make_shared<Callback>(std::move(callback));
    std::string username = req->getParameter("username");
    std::string password = req->getParameter("password");
    if (username.empty() || password.empty())
    {
        (*cb)(textResp("Singup Failed"));
        return;
    }

    // 密码不落明文，存 sha1 密文
    db()->execSqlAsync(
        "INSERT INTO tbl_user (user_name, user_pwd, signup_at) VALUES (?, ?, NOW())",
        [cb](const orm::Result &) { (*cb)(textResp("SUCCESS")); },
        [cb](const orm::DrogonDbException &) { (*cb)(textResp("Singup Failed")); },
        username, Hash::sha1(password));
}

void ApiCtrl::signin(const HttpRequestPtr &req, Callback &&callback)
{
    CallbackPtr cb = std::make_shared<Callback>(std::move(callback));
    std::string username = req->getParameter("username");
    std::string password = req->getParameter("password");
    if (username.empty() || password.empty())
    {
        (*cb)(textResp("Login Failed"));
        return;
    }

    db()->execSqlAsync(
        "SELECT user_pwd FROM tbl_user WHERE user_name = ? AND status = 0",
        [=](const orm::Result &result) {
            if (result.empty() ||
                result[0]["user_pwd"].as<std::string>() != Hash::sha1(password))
            {
                (*cb)(textResp("Login Failed"));
                return;
            }

            std::string token = Token::generate_token(username);
            // REPLACE：重新登录会顶掉旧 token（单点登录态）
            db()->execSqlAsync(
                "REPLACE INTO tbl_user_token (user_name, user_token) VALUES (?, ?)",
                [=](const orm::Result &) {
                    setTokenCache(username, token);
                    Json::Value data;
                    data["data"]["Token"] = token;
                    data["data"]["Username"] = username;
                    data["data"]["Location"] = "/home";
                    (*cb)(HttpResponse::newHttpJsonResponse(data));
                },
                [=](const orm::DrogonDbException &) { (*cb)(textResp("Login Failed")); },
                username, token);
        },
        [cb](const orm::DrogonDbException &) { (*cb)(textResp("Login Failed")); },
        username);
}

void ApiCtrl::userInfo(const HttpRequestPtr &req, Callback &&callback)
{
    CallbackPtr cb = std::make_shared<Callback>(std::move(callback));
    std::string username = req->getParameter("username");
    std::string token = req->getParameter("token");
    if (username.empty() || token.empty())
    {
        (*cb)(textResp("error"));
        return;
    }

    checkToken(username, token, "error", cb, [=]() {
        db()->execSqlAsync(
            "SELECT user_name, signup_at FROM tbl_user WHERE user_name = ? AND status = 0",
            [cb](const orm::Result &result) {
                if (result.empty())
                {
                    (*cb)(textResp("error"));
                    return;
                }
                Json::Value data;
                data["data"]["Username"] = result[0]["user_name"].as<std::string>();
                data["data"]["SignupAt"] = result[0]["signup_at"].as<std::string>();
                (*cb)(HttpResponse::newHttpJsonResponse(data));
            },
            [cb](const orm::DrogonDbException &) { (*cb)(textResp("error")); },
            username);
    });
}

// ---------------- 文件系统 ----------------

void ApiCtrl::fileQuery(const HttpRequestPtr &req, Callback &&callback)
{
    CallbackPtr cb = std::make_shared<Callback>(std::move(callback));
    std::string username = req->getParameter("username");
    std::string token = req->getParameter("token");
    if (username.empty() || token.empty())
    {
        (*cb)(textResp("error"));
        return;
    }

    std::string limit = req->getParameter("limit");
    if (!isNumberString(limit))
        limit = "10";

    checkToken(username, token, "error", cb, [=]() {
        // 取列表缓存版本号：上传成功会 INCR，旧版本的缓存自然失效
        redisCli()->execCommandAsync(
            [=](const nosql::RedisResult &verResult) {
                std::string ver;
                std::string stamp = redisString(verResult, ver) ? ver : "0";

                redisCli()->execCommandAsync(
                    [=](const nosql::RedisResult &listResult) {
                        std::string cached;
                        if (redisString(listResult, cached) && !cached.empty())
                        {
                            // 缓存命中，不再查库
                            auto resp = HttpResponse::newHttpResponse();
                            resp->setBody(cached);
                            resp->setContentTypeCode(CT_APPLICATION_JSON);
                            (*cb)(resp);
                            return;
                        }
                        queryFileListFromDb(username, stamp, limit, cb);
                    },
                    [=](const std::exception &) {
                        queryFileListFromDb(username, stamp, limit, cb);
                    },
                    "GET %s", filelistKey(username, stamp, limit).c_str());
            },
            [=](const std::exception &) {
                // Redis 异常：跳过缓存直接查库
                queryFileListFromDb(username, "0", limit, cb);
            },
            "GET %s", flverKey(username).c_str());
    });
}

void ApiCtrl::fileUpload(const HttpRequestPtr &req, Callback &&callback)
{
    CallbackPtr cb = std::make_shared<Callback>(std::move(callback));
    std::string username = req->getParameter("username");
    std::string token = req->getParameter("token");

    MultiPartParser parser;
    if (username.empty() || token.empty() || parser.parse(req) != 0 ||
        parser.getFiles().empty())
    {
        (*cb)(textResp("Singup Failed"));
        return;
    }

    // 客户端文件名不可信任：去掉路径部分，防目录穿越
    std::string fileName = parser.getFiles()[0].getFileName();
    auto slash = fileName.find_last_of("/\\");
    if (slash != std::string::npos)
        fileName = fileName.substr(slash + 1);

    std::string content(parser.getFiles()[0].fileContent());
    if (fileName.empty() || content.empty())
    {
        (*cb)(textResp("Singup Failed"));
        return;
    }

    std::string sha1 = Hash::sha1(content);
    if (sha1.empty())
    {
        (*cb)(textResp("Singup Failed"));
        return;
    }
    unsigned long long fileSize = content.size();

    checkToken(username, token, "Singup Failed", cb,
               [=, content = std::move(content)]() {
                   // 文件以 sha1 命名落地：相同内容只存一份（去重/秒传基础）
                   std::ofstream ofs(FILES_DIR + sha1,
                                     std::ios::binary | std::ios::trunc);
                   ofs.write(content.data(), (std::streamsize)content.size());
                   ofs.close();

                   db()->execSqlAsync(
                       "INSERT INTO tbl_file (file_sha1, file_name, file_size, file_addr) "
                       "VALUES (?, ?, ?, ?) ON DUPLICATE KEY UPDATE update_at = NOW()",
                       [=](const orm::Result &) {
                           db()->execSqlAsync(
                               "INSERT INTO tbl_user_file "
                               "(user_name, file_sha1, file_size, file_name) "
                               "VALUES (?, ?, ?, ?) ON DUPLICATE KEY UPDATE "
                               "file_size = VALUES(file_size), "
                               "file_name = VALUES(file_name), status = 0, "
                               "last_update = NOW()",
                               [=](const orm::Result &) {
                                   // 上传成功：版本号自增，让列表缓存失效
                                   redisWrite("INCR", {flverKey(username)});
                                   (*cb)(textResp("upload Success"));
                               },
                               [=](const orm::DrogonDbException &) {
                                   (*cb)(textResp("Singup Failed"));
                               },
                               username, sha1, (uint64_t)fileSize, fileName);
                       },
                       [=](const orm::DrogonDbException &) {
                           (*cb)(textResp("Singup Failed"));
                       },
                       sha1, fileName, (uint64_t)fileSize, FILES_DIR + sha1);
               });
}

void ApiCtrl::downloadUrl(const HttpRequestPtr &req, Callback &&callback)
{
    CallbackPtr cb = std::make_shared<Callback>(std::move(callback));
    std::string fileName = req->getParameter("filename");
    if (fileName.empty())
    {
        (*cb)(textResp("error"));
        return;
    }

    db()->execSqlAsync(
        "SELECT f.file_sha1 FROM tbl_user_file AS f JOIN tbl_file AS g "
        "ON f.file_sha1 = g.file_sha1 "
        "WHERE f.file_name = ? AND f.status = 0 AND g.status = 0 LIMIT 1",
        [cb](const orm::Result &result) {
            if (result.empty())
            {
                (*cb)(textResp("error"));
                return;
            }
            (*cb)(textResp(NGINX_URL + "/" +
                           result[0]["file_sha1"].as<std::string>()));
        },
        [cb](const orm::DrogonDbException &) { (*cb)(textResp("error")); },
        fileName);
}

// ---------------- 静态页面 ----------------

void ApiCtrl::spaPage(const HttpRequestPtr &req, Callback &&callback)
{
    callback(spaIndex());
}

void ApiCtrl::avatar(const HttpRequestPtr &req, Callback &&callback)
{
    callback(HttpResponse::newFileResponse("./static/img/avatar.jpeg"));
}
