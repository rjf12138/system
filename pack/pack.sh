#!/bin/bash
# 将 system 库打包成 .pm 并发布到本地仓库
#
# 用法:
#   ./pack.sh            # 编译 debug/release 两套 libsystem.a, 打包并发布
#   ./pack.sh -f         # 同名同版本已存在时强制覆盖
#
# 产物:
#   ~/.project_manager/repo/system-<version>.pm (+ .sha256 摘要边车)
#
# 包内结构:
#   system/
#   ├── manifest.json
#   ├── include/system.h
#   └── lib/
#       ├── debug/libsystem.a
#       └── release/libsystem.a
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PKG_NAME="system"
PKG_VERSION="0.1.2"
WORK_DIR="$ROOT/pack/build"
PKG_DIR="$WORK_DIR/$PKG_NAME"

FORCE_FLAG=""
if [ "$1" = "-f" ]; then
    FORCE_FLAG="-f"
fi

SRCS=(
    "$ROOT/src/directory.cc"
    "$ROOT/src/file.cc"
    "$ROOT/src/mutex.cc"
    "$ROOT/src/socket.cc"
    "$ROOT/src/system.cc"
    "$ROOT/src/systeminfo.cc"
    "$ROOT/src/terminal_window.cc"
    "$ROOT/src/thread.cc"
    "$ROOT/src/time.cc"
)

echo "==> 清理旧的打包产物"
rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR/include" "$PKG_DIR/lib/debug" "$PKG_DIR/lib/release"

build_lib() {
    # $1 = 模式名(debug/release), $2 = 编译 flags
    local mode="$1"
    local flags="$2"
    local obj_dir="$WORK_DIR/obj_$mode"
    mkdir -p "$obj_dir"

    echo "==> 编译 $mode 静态库 (flags: $flags)"
    for src in "${SRCS[@]}"; do
        local base
        base="$(basename "$src" .cc)"
        g++ -std=c++11 $flags \
            -I"$ROOT/inc" -I"$ROOT/extern_inc" -I"$ROOT/src" \
            -c "$src" -o "$obj_dir/$base.o"
    done

    ar rcs "$PKG_DIR/lib/$mode/lib$PKG_NAME.a" "$obj_dir"/*.o
    echo "    -> $PKG_DIR/lib/$mode/lib$PKG_NAME.a"
}

# debug: 调试信息、不做优化
build_lib debug "-g -O0 -DDEBUG"

# release: 可移植 flags(不用 -march=native, 避免绑定构建机 CPU)
build_lib release "-O2 -mtune=generic -DNDEBUG"

echo "==> 拷贝头文件与 manifest"
cp "$ROOT"/inc/*.h "$PKG_DIR/include/"
cp "$ROOT/pack/manifest.json" "$PKG_DIR/"

echo "==> 打包并发布到本地仓库"
pm pkg create "$PKG_DIR" $FORCE_FLAG

echo "==> 完成, 仓库内容:"
pm pkg list | grep -E "$PKG_NAME|name" || true
