#!/bin/bash

# === 默认配置 ===
BASE_BRANCH="main"
BASE_NAME="customer"
MAJOR_VERSION=""

# === 参数解析 ===
while getopts "v:" opt; do
    case $opt in
        v) MAJOR_VERSION=$OPTARG ;;
        *) echo "使用方法: $0 -v <主版本号>"; exit 1 ;;
    esac
done

if [ -z "$MAJOR_VERSION" ]; then
    echo "请指定主版本号，例如: $0 -v 2"
    exit 1
fi

# === 查找当前最大版本 ===
echo "正在查找已有分支: ${BASE_NAME}_v${MAJOR_VERSION}.*"

latest_minor=$(git branch -r | \
    grep "origin/${BASE_NAME}_v${MAJOR_VERSION}\." | \
    sed -E "s|.*/${BASE_NAME}_v${MAJOR_VERSION}\.||" | \
    sort -V | \
    tail -n1)

if [[ -z "$latest_minor" ]]; then
    next_minor=0
else
    next_minor=$((latest_minor + 1))
fi

SDK_RELEASE_BRANCH="${BASE_NAME}_v${MAJOR_VERSION}.${next_minor}"
echo "创建新发布分支: $SDK_RELEASE_BRANCH"

# === 开始构建分支 ===
git checkout $BASE_BRANCH || { echo "切换到 $BASE_BRANCH 失败"; exit 1; }
git checkout -b $SDK_RELEASE_BRANCH || { echo "创建分支失败"; exit 1; }

# === 删除内部模块 ===
rm -rf alg_cal
rm -rf inc/dev
rm -rf inc/calibration
rm -rf src/dev
rm -rf src/calibration
rm -rf src/data_base

# === 删除 DTOF_SDK_DEVELOPER_MODE 宏 ===
DTOF_COMMON_HEADER="inc/dtof_common.h"

if [ -f "$DTOF_COMMON_HEADER" ]; then
    echo "移除 DTOF_SDK_DEVELOPER_MODE 宏定义"
    sed -i '/^[[:space:]]*#define[[:space:]]\+DTOF_SDK_DEVELOPER_MODE/d' "$DTOF_COMMON_HEADER"
else
    echo "未找到 $DTOF_COMMON_HEADER"
    exit 1
fi

# === 提交发布 ===
git add .
git commit -m "release: strip internal for $SDK_RELEASE_BRANCH"
git push origin $SDK_RELEASE_BRANCH

# === 打包 ZIP ===
git archive --format=zip --output="sdk_${SDK_RELEASE_BRANCH}.zip" HEAD $(git ls-tree -r --name-only HEAD | grep -v 'generate_sdk.sh')
echo "打包完成: sdk_${SDK_RELEASE_BRANCH}.zip"
echo "SDK 发布分支已完成: $SDK_RELEASE_BRANCH"
