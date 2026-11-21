#!/usr/bin/env bash
# build-macos.sh — 在 macOS 上构建并打包 dmg。
# CMake 已接入 qt_generate_deploy_qml_app_script，安装时自动执行
# 等价于 macdeployqt 的收编（Qt 框架、插件、QML 导入都进 .app）。
#
# 用法：
#   QTDIR=$HOME/Qt/6.8.0/macos ./scripts/build-macos.sh
set -euo pipefail

SRC="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${BUILD_DIR:-$SRC/build-macos}"
PREFIX="${PREFIX:-$SRC/dist/macos-staging}"
DMG="$SRC/dist/gdei-connect-macos.dmg"

CMAKE_ARGS=(-S "$SRC" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX")
if [ -n "${QTDIR:-}" ]; then
  CMAKE_ARGS+=(-DCMAKE_PREFIX_PATH="$QTDIR")
fi

echo "==> 配置 Configure"
cmake "${CMAKE_ARGS[@]}"

echo "==> 编译 Build"
cmake --build "$BUILD"

echo "==> 部署 (macdeployqt 等效步骤由 CMake 自动完成)"
rm -rf "$PREFIX"
cmake --install "$BUILD"

APP="$(find "$PREFIX" -maxdepth 2 -name 'gdei-connect.app' | head -1)"
if [ -z "$APP" ]; then
  echo "ERROR: gdei-connect.app not found under $PREFIX" >&2
  exit 1
fi

echo "==> 制作 dmg"
mkdir -p "$(dirname "$DMG")"
rm -f "$DMG"
hdiutil create -volname "gdei-connect" -srcfolder "$APP" -ov -format UDZO "$DMG"

echo
echo "完成: $DMG"
echo "打开 dmg 后把 gdei-connect.app 拖入应用程序即可运行。"
