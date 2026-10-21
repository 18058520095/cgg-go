#!/usr/bin/env bash
# 本地CI脚本：与 .github/workflows/ci.yml 相同的构建+测试流程
# 可在任意环境运行（包括无法访问GitHub的环境 / Gitee）
set -euo pipefail
cd "$(dirname "$0")/.."

BUILD_DIR=${1:-build-ci}

echo "==> Configure (Release)"
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release

echo "==> Build"
cmake --build "$BUILD_DIR" -j

echo "==> Test"
ctest --test-dir "$BUILD_DIR" --output-on-failure

echo "==> CI passed"
