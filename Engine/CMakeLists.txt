# ============================================================================
# Engine 子目录 CMake 构建配置
#
# 功能:
#   1. 收集 src/ 下所有 .cpp 和 .h 文件作为引擎静态库源文件
#   2. 编译 Engine 静态库，链接 SDL2 和 SDL2_mixer
#   3. 编译主可执行文件 OpenRhythmEngine.exe（链接 Engine 库）
#
# 构建产物:
#   - Engine.lib (静态库)
#   - OpenRhythmEngine.exe (可执行文件)
# ============================================================================

# ---------- 收集源文件 ----------
# GLOB_RECURSE: 递归搜索 src/ 目录下所有 .cpp 和 .h 文件
file(GLOB_RECURSE ENGINE_SOURCES src/*.cpp)
file(GLOB_RECURSE ENGINE_HEADERS src/*.h)

# ---------- 静态库目标 ----------
# 将引擎核心代码编译为静态库，方便后续编辑器等子项目复用
add_library(Engine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})

# 设置 Engine 库的公开头文件搜索路径
# 其他目标链接 Engine 后自动获得此路径
target_include_directories(Engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 链接 SDL2 和 SDL2_mixer
# PUBLIC 意味着链接 Engine 的目标也会自动链接这些库
target_link_libraries(Engine PUBLIC
    SDL2::SDL2
    SDL2_mixer::SDL2_mixer
)

# ---------- 可执行文件目标 ----------
# 主游戏程序入口
add_executable(OpenRhythmEngine
    ${CMAKE_SOURCE_DIR}/main.cpp   # main.cpp 在项目根目录
)

# 链接 Engine 静态库和 SDL2 main（Windows 上 SDL 需要 SDL2main 处理入口点）
target_link_libraries(OpenRhythmEngine PRIVATE Engine SDL2::SDL2main)
