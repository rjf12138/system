# ncurses 6.2 打包说明（pm 包）

本目录把 system 项目内已有的 ncurses 预编译产物打成 pm 包：

- 头文件：`extern_inc/ncurses/*.h`
- debug 库：`lib/debug/ncurses/*_g.a`（打包时归一化为 `libncurses.a` 等标准名字）
- release 库：`lib/release/ncurses/*.a`

## 打包

```bash
./pack.sh        # 打包并发布到 ~/.project_manager/repo/
./pack.sh -f     # 同名同版本已存在时强制覆盖
```

产物：

- `~/.project_manager/repo/ncurses-6.2.pm`
- `~/.project_manager/repo/ncurses-6.2.pm.sha256`

包内结构：

```text
ncurses/
├── manifest.json                 # link_libs = ncurses++, form, menu, panel, ncurses
├── include/*.h                   # 安装到 extern_inc/ncurses/
└── lib/
    ├── debug/libncurses.a ...    # 由 *_g.a 归一化而来
    └── release/libncurses.a ...
```

## 使用

```bash
pm deps add ncurses -p MyApp
```

```cpp
#include <ncurses/ncurses.h>
```

system 项目已经声明 `ncurses@6.2` 依赖，安装 system 时会自动安装 ncurses。

> 包内不包含 terminfo，运行时使用系统自身的 terminfo 数据库。
