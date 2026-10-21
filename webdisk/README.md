# Cloudisk 网盘

一个基于 C++（Drogon 框架）+ Vue3 的 Web 网盘项目，前端为单页应用，后端提供 REST 接口，下载流量走 nginx 静态服务。

## 技术栈

| 层 | 技术 |
|---|---|
| 后端 | C++17 · Drogon（异步 Web 框架）· OpenSSL（SHA1） |
| 数据库 | MySQL 8.0 · Redis 7.0 |
| 静态下载 | nginx 1.24 |
| 前端 | Vue3 · Vite · Vue Router· Pinia · axios |

## 项目结构

```
webdisk/
├── drogon_server/          # 后端
│   ├── main.cc             # 启动、连接池、监听端口
│   ├── ApiCtrl.h/.cc       # 接口实现（HttpController）
│   ├── CMakeLists.txt
│   └── build/              # 构建产物
├── Hash.h/.cc              # SHA1（密码、文件指纹）
├── Token.h/.cc             # 登录 token 生成
├── frontend/               # 前端 Vue3 工程
│   ├── src/                # 页面、路由、store、api
│   └── dist/               # 构建产物（后端直接托管）
├── static/img/avatar.jpeg  # 默认头像
├── tmp/                    # 上传文件落地 + nginx 下载根目录
├── file.conf               # nginx 下载服务配置
└── tbl_sql.sql             # 建库建表脚本
```

## 接口

| 方法 | 路径 | 说明 | 响应 |
|---|---|---|---|
| POST | `/user/signup` | 注册（urlencoded） | `SUCCESS` / `Singup Failed` |
| POST | `/user/signin` | 登录（urlencoded） | `{data:{Token,Username,Location}}` |
| POST | `/user/sigin` | 兼容文档笔误，同登录 | 同上 |
| GET | `/user/info` | 用户信息 | `{data:{Username,SignupAt}}` |
| POST | `/file/query` | 文件列表（body 带 limit） | `[{FileHash,FileName,...}]` |
| POST | `/file/upload` | 上传（multipart，key=file） | `upload Success` |
| GET | `/file/downloadurl` | 获取下载地址 | nginx URL |

## 核心设计

- **登录态**：登录成功后签发 token（用户名+时间戳+随机数 SHA1），写入 `tbl_user_token` 持久层 + Redis 缓存（`cloudisk:token:<user>`，SETEX 3600）。校验先查缓存、未命中回退 MySQL。`REPLACE INTO` 实现重新登录顶掉旧 token（单点登录）。
- **文件去重/秒传**：文件内容 SHA1 作为存储文件名，相同内容只落一份，`tbl_file` 为全局文件表，`tbl_user_file` 记录用户与文件关系。
- **列表缓存**：Redis 存当前版本号 `cloudisk:flver:<user>`，列表缓存 key 带版本号与 limit（`cloudisk:filelist:<user>:<ver>:<limit>`，SETEX 60）。上传成功后 `INCR` 版本号，旧缓存自然失效，避免 SCAN 删除。
- **缓存容错**：Redis 仅作加速层，异常时全部回退 MySQL 并回填，不影响业务结果。
- **下载**：后端只返回 nginx 地址 `http://127.0.0.1:8080/<sha1>`，文件内容由 nginx 从 `tmp/` 直接分发。

## 运行

依赖：MySQL、Redis、nginx、Drogon 1.8+（`libdrogon-dev`）、OpenSSL。前端构建需 Node 18+。

```bash
# 0. 依赖服务
service mysql start
redis-server --daemonize yes
nginx -c /root/go_lib/webdisk/file.conf        # 8080 下载服务

# 1. 首次建库（脚本内含 CREATE DATABASE / USE cloudisk）
mysql -uroot -ppassword -h127.0.0.1 < tbl_sql.sql

# 2. 前端构建
cd frontend
npm install && npm run build                    # 产出 dist/

# 3. 后端编译 + 启动（运行必须在 webdisk 根目录，代码中的路径为相对路径）
cd /root/go_lib/webdisk
cmake -S drogon_server -B drogon_server/build -DCMAKE_BUILD_TYPE=Release   # -S 指定源码目录
cmake --build drogon_server/build -j8
./drogon_server/build/cloudisk_drogon           # 前端已由后端托管

# 4. 访问
# http://127.0.0.1:1234/login
```

## 安全提示

- 密码以 SHA1 密文存储，生产环境建议改用 bcrypt/Argon2 慢哈希。