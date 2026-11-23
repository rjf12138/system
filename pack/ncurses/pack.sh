#!/bin/bash
# 将 system 内置的 ncurses 6.2 预编译库打成 pm 包
#
# 用法:
#   ./pack.sh            # 打包并发布到本地仓库
#   ./pack.sh -f         # 同名同版本已存在时强制覆盖
#
# 产物:
#   ~/.project_manager/repo/ncurses-6.2.pm (+ .sha256)
#
# debug 库原始文件名带 _g 后缀，这里归一化成和 release 一致的库名，
# 使 deps.libs 中同一组 -l 名字在 debug/release 下都能使用。
set -e

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PKG_NAME="ncurses"
PKG_VERSION="6.2"
WORK_DIR="$ROOT/pack/build"
PKG_DIR="$WORK_DIR/$PKG_NAME"

FORCE_FLAG=""
if [ "$1" = "-f" ]; then
    FORCE_FLAG="-f"
fi

echo "==> 清理旧的打包产物"
rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR/include" "$PKG_DIR/lib/debug" "$PKG_DIR/lib/release"

echo "==> 拷贝 ncurses 头文件"
cp "$ROOT"/extern_inc/ncurses/*.h "$PKG_DIR/include/"

echo "==> 归一化 debug 库名并拷贝 release 库"
cp "$ROOT/lib/debug/ncurses/libncurses_g.a"   "$PKG_DIR/lib/debug/libncurses.a"
cp "$ROOT/lib/debug/ncurses/libncurses++_g.a" "$PKG_DIR/lib/debug/libncurses++.a"
cp "$ROOT/lib/debug/ncurses/libform_g.a"      "$PKG_DIR/lib/debug/libform.a"
cp "$ROOT/lib/debug/ncurses/libmenu_g.a"      "$PKG_DIR/lib/debug/libmenu.a"
cp "$ROOT/lib/debug/ncurses/libpanel_g.a"     "$PKG_DIR/lib/debug/libpanel.a"
cp "$ROOT"/lib/release/ncurses/*.a            "$PKG_DIR/lib/release/"

echo "==> 拷贝 manifest 并打包"
cp "$ROOT/pack/ncurses/manifest.json" "$PKG_DIR/"
pm pkg create "$PKG_DIR" $FORCE_FLAG

echo "==> 完成, 仓库内容:"
pm pkg list | grep -E "$PKG_NAME|name" || true
