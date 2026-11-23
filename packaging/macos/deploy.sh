#!/bin/bash
# packaging/macos/deploy.sh
# macOS 部署打包：macdeployqt 收编框架与 QML 插件，再制作 dmg 镜像。
# macOS packaging: macdeployqt bundles frameworks and QML plugins, then builds a dmg.
set -euo pipefail

BUILD_DIR="${1:-build}"
DIST_DIR="${2:-artifacts/macos}"

mkdir -p "$DIST_DIR"

app="$(find "$BUILD_DIR" -name 'gdei-auth-client.app' -maxdepth 4 | head -1)"
if [ -z "$app" ]; then
    echo "ERROR: gdei-auth-client.app not found under $BUILD_DIR"
    exit 1
fi

macdeployqt "$app"

# 从 assets/app.iconset 生成 bundle 图标 app.icns 并放入 Contents/Resources。
# Build the bundle icon app.icns from assets/app.iconset into Contents/Resources.
iconset="$(cd "$(dirname "$0")/../../assets/app.iconset" && pwd)"
if [ -d "$iconset" ]; then
    mkdir -p "$app/Contents/Resources"
    iconutil -c icns "$iconset" -o "$app/Contents/Resources/app.icns"
fi

staging="$DIST_DIR/dmg-staging"
rm -rf "$staging"
mkdir -p "$staging"
cp -R "$app" "$staging/"

hdiutil create -volname "gdei-auth-client" -srcfolder "$staging" -ov -format UDZO \
    "$DIST_DIR/gdei-auth-client-macos.dmg"

rm -rf "$staging"
