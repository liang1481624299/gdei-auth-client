#!/usr/bin/env bash
# build-linux.sh — 构建并打包一个自包含的 Linux 运行目录（Debian / Arch 通用）。
#
# 为什么这样做：CMake 里已接入 qt_generate_deploy_qml_app_script，
# `cmake --install` 会把 Qt 共享库、平台插件、QML 导入模块一起部署到安装前缀，
# 再配上 qt.conf + 启动脚本，就能在任何带 glibc 的 x86_64 Linux 上双击运行，
# 不依赖目标机是否装了 Qt 系统包（修复"装上 deb 也打不开"的问题）。
#
# 用法：
#   QTDIR=/opt/Qt/6.8.0/gcc_64 ./scripts/build-linux.sh
# 若用系统 Qt（Debian / Arch 自带 qt6-*），可省略 QTDIR：
#   ./scripts/build-linux.sh
set -euo pipefail

SRC="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${BUILD_DIR:-$SRC/build-linux}"
PREFIX="${PREFIX:-$SRC/artifacts/arch/gdei-auth-client-linux-x64}"
ARCHIVE="$SRC/artifacts/arch/gdei-auth-client-linux-x64.tar.gz"

CMAKE_ARGS=(-S "$SRC" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX")
if [ -n "${QTDIR:-}" ]; then
  CMAKE_ARGS+=(-DCMAKE_PREFIX_PATH="$QTDIR")
fi

echo "==> 配置 Configure"
cmake "${CMAKE_ARGS[@]}"

echo "==> 编译 Build"
cmake --build "$BUILD"

echo "==> 部署到 $PREFIX (Qt 库 / 插件 / QML 由 CMake 自动部署)"
rm -rf "$PREFIX"
cmake --install "$BUILD"

# qt.conf：让 exe 从相对路径找到插件与 QML（位于 bin/ 旁）
cat > "$PREFIX/bin/qt.conf" <<'EOF'
[Paths]
Prefix=..
Plugins=plugins
Qml2Imports=qml
LibraryExecutables=libexec
EOF

# 启动器：设置运行时库路径，保证脱离终端也能跑
cat > "$PREFIX/gdei-auth-client.sh" <<EOF
#!/usr/bin/env bash
DIR="\$(cd "\$(dirname "\$0")" && pwd)"
export QT_PLUGIN_PATH="\$DIR/plugins"
export QML2_IMPORT_PATH="\$DIR/qml"
export LD_LIBRARY_PATH="\$DIR/lib:\${LD_LIBRARY_PATH:-}"
exec "\$DIR/bin/gdei-auth-client" "\$@"
EOF
chmod +x "$PREFIX/gdei-auth-client.sh"

echo "==> 归档 Archive"
mkdir -p "$(dirname "$ARCHIVE")"
tar -czf "$ARCHIVE" -C "$SRC/artifacts/arch" "gdei-auth-client-linux-x64"

echo
echo "完成。运行目录: $PREFIX"
echo "启动: $PREFIX/gdei-auth-client.sh"
echo "归档: $ARCHIVE"
echo "（解压后双击 gdei-auth-client.sh 或在终端运行即可，无需安装 Qt）"
