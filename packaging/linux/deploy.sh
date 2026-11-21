#!/usr/bin/env bash
# packaging/linux/deploy.sh
# 将已构建的产物整理为一个【自包含】的 Linux 运行目录，并归档为 tar.gz。
#
# 目标：解压后在任何带 glibc 的 x86_64 Linux（Debian / Arch 等）上运行，
#       不需要目标机预装 Qt —— 修复“装上也打不开”的根因（之前只拷裸二进制）。
#
# 原理：CMakeLists 已接入 qt_generate_deploy_qml_app_script，
#       `cmake --install` 会把 Qt 共享库、平台插件、QML 导入模块以及运行时依赖闭包
#       一起部署到安装前缀；再配 qt.conf + 启动器，即可在非 Qt 安装前缀下解析插件/QML。
#
# 前置：BUILD_DIR 必须是已经用 CMake + Qt6 配置并编译过的构建目录，例如：
#       cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
#             [-DCMAKE_PREFIX_PATH=/opt/Qt/6.8.0/gcc_64]
#       cmake --build build
#
# 用法：
#   ./packaging/linux/deploy.sh [BUILD_DIR] [DIST_DIR]
set -euo pipefail

BUILD_DIR="${1:-build}"
DIST_DIR="${2:-dist}"
STAGING="$DIST_DIR/gdei-connect-linux-x64"
ARCHIVE="$DIST_DIR/gdei-connect-linux-x64.tar.gz"

if [ ! -d "$BUILD_DIR" ]; then
    echo "ERROR: 构建目录不存在: $BUILD_DIR" >&2
    echo "  请先配置并编译，例如：" >&2
    echo "    cmake -S . -B $BUILD_DIR -G Ninja -DCMAKE_BUILD_TYPE=Release [-DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64]" >&2
    echo "    cmake --build $BUILD_DIR" >&2
    exit 1
fi

mkdir -p "$DIST_DIR"
rm -rf "$STAGING"

echo "==> 部署到 $STAGING (cmake --install 自动收编 Qt 库 / 插件 / QML / 运行时依赖)"
cmake --install "$BUILD_DIR" --prefix "$STAGING"

BIN="$STAGING/bin"
if [ ! -x "$BIN/gdei-connect" ]; then
    echo "ERROR: 未找到可执行文件: $BIN/gdei-connect" >&2
    exit 1
fi

# qt.conf：让 exe 从相对路径定位插件与 QML（放在 bin/ 旁，Prefix 指向上一级目录）
cat > "$BIN/qt.conf" <<'EOF'
[Paths]
Prefix=..
Plugins=plugins
Qml2Imports=qml
LibraryExecutables=libexec
EOF

# 启动器：设置运行时库路径，保证脱离终端 / 双击也能跑
cat > "$STAGING/gdei-connect.sh" <<EOF
#!/usr/bin/env bash
DIR="\$(cd "\$(dirname "\$0")" && pwd)"
export QT_PLUGIN_PATH="\$DIR/plugins"
export QML2_IMPORT_PATH="\$DIR/qml"
export LD_LIBRARY_PATH="\$DIR/lib:\${LD_LIBRARY_PATH:-}"
exec "\$DIR/bin/gdei-connect" "\$@"
EOF
chmod +x "$STAGING/gdei-connect.sh"

echo "==> 归档 $ARCHIVE"
tar -czf "$ARCHIVE" -C "$DIST_DIR" "$(basename "$STAGING")"

echo
echo "完成。"
echo "  自包含目录: $STAGING"
echo "  归档文件:   $ARCHIVE"
echo "  启动方式:   解压后运行 $STAGING/gdei-connect.sh （无需安装 Qt）"
