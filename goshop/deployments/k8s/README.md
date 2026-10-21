# GoShop Kubernetes 部署说明

## 前置条件
- 已安装 kubectl 并配置好集群
- 集群中已安装 nginx-ingress-controller（如使用 Ingress）

## 部署步骤

### 1. 构建并推送镜像
```bash
docker build -f deployments/docker/Dockerfile -t goshop:latest .
```

### 2. 按顺序部署
```bash
kubectl apply -f deployments/k8s/namespace.yaml
kubectl apply -f deployments/k8s/configmap.yaml
kubectl apply -f deployments/k8s/mysql.yaml
kubectl apply -f deployments/k8s/redis.yaml
kubectl apply -f deployments/k8s/app.yaml
```

### 3. 验证部署
```bash
kubectl get pods -n goshop
kubectl get svc -n goshop
kubectl logs -f deployment/goshop-app -n goshop
```

### 4. 访问服务

**方式一：端口转发**
```bash
kubectl port-forward svc/goshop-app 8080:8080 -n goshop
```

**方式二：Ingress（配置 hosts 后访问 http://goshop.local）**

## 扩缩容
```bash
kubectl scale deployment goshop-app --replicas=3 -n goshop
```

## 清理
```bash
kubectl delete namespace goshop
```
