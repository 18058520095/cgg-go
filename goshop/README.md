# GoShop — 基于 Gin 的电商后端

使用 Go + Gin + GORM 构建的电商后端系统，覆盖用户认证、商品、购物车、订单核心模块，提供 Docker Compose 与 Kubernetes 部署方案。

## 技术栈

| 层级 | 技术 |
|------|------|
| Web 框架 | Gin |
| ORM | GORM |
| 数据库 | MySQL 8.0 |
| 缓存 | Redis 7 |
| 认证 | JWT (golang-jwt) |
| 密码加密 | bcrypt |
| 容器化 | Docker + Docker Compose |
| 编排 | Kubernetes |

## 项目结构

```
goshop/
├── cmd/server/              # 入口 main
├── internal/
│   ├── config/              # 配置加载
│   ├── database/            # MySQL & Redis 连接
│   ├── middleware/          # 中间件（JWT/CORS/Recovery/Logger）
│   ├── models/              # GORM 数据模型
│   ├── modules/             # 业务模块
│   │   ├── auth/            # 认证（注册/登录/用户信息）
│   │   ├── product/         # 商品（CRUD/列表/搜索）
│   │   ├── cart/            # 购物车
│   │   └── order/           # 订单（事务处理）
│   ├── router/              # 路由注册
│   ├── webui/               # 内嵌前端单页（HTML+JS，零依赖）
│   └── utils/               # 工具（JWT/密码/响应封装）
├── deployments/
│   ├── docker/              # Dockerfile & init.sql
│   └── k8s/                 # K8s 部署清单
├── docker-compose.yml
└── .env.example
```

## 分层架构

```
请求 → Middleware → Handler(Controller) → Service → GORM → DB
```

## 快速开始

### 方式一：本地运行

```bash
mysql -uroot -p -e "CREATE DATABASE goshop DEFAULT CHARACTER SET utf8mb4;"
cp .env.example .env
go mod tidy
go run ./cmd/server
```

### 方式二：Docker Compose（推荐）

```bash
docker compose up -d --build
```

## 前端页面

项目内置一个零依赖单页商城前端（`internal/webui/index.html`，通过 `go:embed` 打进二进制，无需额外静态资源）。

启动服务后，浏览器打开 `http://localhost:8090/` 即可使用：

- 右上角「登录 / 注册」→ 登录后获得 Token（保存在 localStorage）
- 商品区：搜索、分页、加入购物车
- 购物车抽屉：修改数量、删除、勾选结算
- 我的订单：查看订单列表、取消未支付订单（库存自动恢复）
- 管理员账号登录后，工具栏会出现「上架商品」按钮

## API 接口

统一响应格式：`{ "code": 0, "message": "success", "data": {...} }`

### 认证（公开）
| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/v1/auth/register` | 注册 |
| POST | `/api/v1/auth/login` | 登录 |

### 用户信息（需 Bearer Token）
| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/v1/auth/me` | 当前用户信息 |

### 商品
| 方法 | 路径 | 说明 | 权限 |
|------|------|------|------|
| GET | `/api/v1/products` | 列表（keyword/category_id/page/page_size） | 公开 |
| GET | `/api/v1/products/:id` | 详情 | 公开 |
| POST | `/api/v1/admin/products` | 创建 | 管理员 |
| PUT | `/api/v1/admin/products/:id` | 更新 | 管理员 |
| DELETE | `/api/v1/admin/products/:id` | 删除 | 管理员 |

### 购物车（需登录）
| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/v1/cart` | 列表 |
| POST | `/api/v1/cart` | 添加 `{ product_id, quantity }` |
| PUT | `/api/v1/cart/:id` | 修改数量 `{ quantity }` |
| DELETE | `/api/v1/cart/:id` | 移除 |

### 订单（需登录）
| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/v1/orders` | 创建订单（从购物车结算） |
| GET | `/api/v1/orders` | 订单列表 |
| GET | `/api/v1/orders/:id` | 订单详情 |
| POST | `/api/v1/orders/:id/cancel` | 取消订单 |

订单创建在一个数据库事务中完成：**校验库存 → 扣减库存 → 创建订单+明细 → 清空已结算购物车**，任一步骤失败则全部回滚。取消订单时会恢复库存。

## Kubernetes 部署

详见 [deployments/k8s/README.md](deployments/k8s/README.md)。

## 安全说明
- 生产环境务必修改 `JWT_SECRET`
- MySQL/Redis 密码应通过 Secret 管理
- 生产环境应启用 HTTPS
