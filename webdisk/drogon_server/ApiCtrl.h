#pragma once

#include <drogon/HttpController.h>

class ApiCtrl : public drogon::HttpController<ApiCtrl>
{
public:
    METHOD_LIST_BEGIN
    // ---- 业务接口 ----
    ADD_METHOD_TO(ApiCtrl::signup, "/user/signup", drogon::Post);
    ADD_METHOD_TO(ApiCtrl::signin, "/user/signin", drogon::Post);
    ADD_METHOD_TO(ApiCtrl::signin, "/user/sigin", drogon::Post);  // 兼容文档笔误
    ADD_METHOD_TO(ApiCtrl::userInfo, "/user/info", drogon::Get);
    ADD_METHOD_TO(ApiCtrl::fileQuery, "/file/query", drogon::Post);
    ADD_METHOD_TO(ApiCtrl::fileUpload, "/file/upload", drogon::Post);
    ADD_METHOD_TO(ApiCtrl::downloadUrl, "/file/downloadurl", drogon::Get);

    // ---- SPA 页面（GET 返回 index.html，交给 Vue Router 处理）----
    ADD_METHOD_TO(ApiCtrl::spaPage, "/", drogon::Get);
    ADD_METHOD_TO(ApiCtrl::spaPage, "/login", drogon::Get);
    ADD_METHOD_TO(ApiCtrl::spaPage, "/register", drogon::Get);
    ADD_METHOD_TO(ApiCtrl::spaPage, "/home", drogon::Get);
    ADD_METHOD_TO(ApiCtrl::spaPage, "/upload", drogon::Get);
    // 兼容课程文档中的旧页面路径
    ADD_METHOD_TO(ApiCtrl::spaPage, "/user/signup", drogon::Get);
    ADD_METHOD_TO(ApiCtrl::spaPage, "/user/signin", drogon::Get);
    ADD_METHOD_TO(ApiCtrl::spaPage, "/user/sigin", drogon::Get);
    ADD_METHOD_TO(ApiCtrl::spaPage, "/file/upload", drogon::Get);
    ADD_METHOD_TO(ApiCtrl::spaPage, "/static/view/signup.html", drogon::Get);
    ADD_METHOD_TO(ApiCtrl::spaPage, "/static/view/signin.html", drogon::Get);
    ADD_METHOD_TO(ApiCtrl::spaPage, "/static/view/home.html", drogon::Get);
    ADD_METHOD_TO(ApiCtrl::spaPage, "/static/view/index.html", drogon::Get);

    // ---- 头像 ----
    ADD_METHOD_TO(ApiCtrl::avatar, "/static/img/avatar.jpeg", drogon::Get);
    METHOD_LIST_END

    void signup(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void signin(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void userInfo(const drogon::HttpRequestPtr &req,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void fileQuery(const drogon::HttpRequestPtr &req,
                   std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void fileUpload(const drogon::HttpRequestPtr &req,
                    std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void downloadUrl(const drogon::HttpRequestPtr &req,
                     std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void spaPage(const drogon::HttpRequestPtr &req,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void avatar(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
