cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(system)

set(CMAKE_DEBUG_POSTFIX d)

set(CXX_FLAGS
    -g
    # -DVALGRIND
    -DCHECK_PTHREAD_RETURN_VALUE
    -D_FILE_OFFSET_BITS=64
    -march=native
    # -MMD
    -rdynamic
    )
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

# 设置生成文件输出路径
set(EXECUTABLE_OUTPUT_PATH, ./output/debug/bin)
add_executable(system ./main/directory_test.cc)

target_include_directories(system PUBLIC ./inc/)
target_include_directories(system PRIVATE ./extern_inc/)

target_link_directories(system PRIVATE ./lib/debug/)
target_link_directories(system PRIVATE ./lib/debug/ncurses)

target_link_libraries(system PRIVATE pthread)
target_link_libraries(system PRIVATE basicd)
target_link_libraries(system PRIVATE ncurses++_g)
target_link_libraries(system PRIVATE form_g)
target_link_libraries(system PRIVATE menu_g)
target_link_libraries(system PRIVATE panel_g)
target_link_libraries(system PRIVATE ncurses_g)
include(src/CMakeLists.txt)

