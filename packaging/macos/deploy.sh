#!/bin/bash
# packaging/macos/deploy.sh
# macOS 部署打包：macdeployqt 收编框架与 QML 插件，再制作 dmg 镜像。
# macOS packaging: macdeployqt bundles frameworks and QML plugins, then builds a dmg.
set -euo pipefail

BUILD_DIR="${1:-build}"
DIST_DIR="${2:-dist}"

mkdir -p "$DIST_DIR"

app="$(find "$BUILD_DIR" -name 'gdei-connect.app' -maxdepth 4 | head -1)"
if [ -z "$app" ]; then
    echo "ERROR: gdei-connect.app not found under $BUILD_DIR"
    exit 1
fi

macdeployqt "$app"

staging="$DIST_DIR/dmg-staging"
rm -rf "$staging"
mkdir -p "$staging"
cp -R "$app" "$staging/"

hdiutil create -volname "gdei-connect" -srcfolder "$staging" -ov -format UDZO \
    "$DIST_DIR/gdei-connect-macos.dmg"

rm -rf "$staging"
