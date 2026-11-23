# system 库打包说明（pm 包）

## 打包

```bash
./pack.sh        # 编译 debug/release 两套 libsystem.a，打包并发布到 ~/.project_manager/repo/
./pack.sh -f     # 同名同版本已存在时强制覆盖（本地迭代用）
```

先确保依赖包已经发布：

```bash
cd ../data_structure && ./pack/pack.sh
cd ../system && ./pack/ncurses/pack.sh
```

产物：

- `~/.project_manager/repo/system-0.1.2.pm`
- `~/.project_manager/repo/system-0.1.2.pm.sha256`

包内结构：

```text
system/
├── manifest.json            # link_libs=[system]，依赖 basic/data_structure/ncurses
├── include/system.h         # 安装到 extern_inc/system/
└── lib/
    ├── debug/libsystem.a    # -g -O0
    └── release/libsystem.a  # -O2 -mtune=generic
```

`data_structure` 头文件不再随 system 包复制，而是通过 manifest 依赖自动安装；
`ncurses` 也通过依赖自动安装，system 包内不重复携带。

## 其它项目依赖 system

```bash
pm deps add system -p MyApp
```

安装后会自动完成：

- `extern_inc/system/system.h`
- `extern_inc/basic/*.h`
- `extern_inc/data_structure/*.h`
- `extern_inc/ncurses/*.h`
- `lib/{debug,release}/libsystem.a`、`libbasic.a`、ncurses 系列库
- `.pmconfig/config.json` 写入依赖和链接库

代码中：

```cpp
#include <system/system.h>
```

## 测试

```bash
pm deps install system
pm build system
```

会构建静态库和 8 个测试可执行文件。`regression_test` 覆盖 File/Directory/Time/
Mutex/Socket/ThreadPool 的关键回归项；`directory_test`、`file_test` 可直接运行；
`mutex_test`、`threadpool_test` 是交互式测试，`echo_client_test`/`echo_server_test`
需要客户端/服务端配合，`dns` 依赖网络环境。

## 升级版本

1. 修改 `.pmconfig/config.json`、`pack/manifest.json`、`pack/pack.sh` 中的版本号；
2. 执行 `./pack.sh`；
3. 使用方执行 `pm pkg install system@<新版> -p MyApp`。
