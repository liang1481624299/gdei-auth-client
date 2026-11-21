#!/bin/bash
# packaging/linux/deploy.sh
# Linux 部署打包：当前先归档二进制 tar.gz。
# deb / rpm / Arch pkg.tar.zst 需在对应发行版容器内用 CPack / 打包脚本生成，
# 系统级依赖（glibc / X11 / Wayland / dbus 等）由目标设备提供，不静态打包。
# Linux packaging: binary tar.gz for now. deb/rpm/Arch packages are built in
# matching distro containers; system libraries stay on the target device.
set -euo pipefail

BUILD_DIR="${1:-build}"
DIST_DIR="${2:-dist}"

mkdir -p "$DIST_DIR"

bin="$(find "$BUILD_DIR" -type f -name 'gdei-connect' | head -1)"
if [ -z "$bin" ]; then
    echo "ERROR: gdei-connect binary not found under $BUILD_DIR"
    exit 1
fi

cp "$bin" "$DIST_DIR/gdei-connect"
tar -czf "$DIST_DIR/gdei-connect-linux-x64.tar.gz" -C "$DIST_DIR" gdei-connect
