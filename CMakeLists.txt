# ============================================================================
# Open Rhythm Engine - 顶层 CMake 构建配置
# 
# 功能: 定义项目级构建设置，查找SDL2依赖，引入Engine子目录
# 要求: CMake 3.16+, C++17, SDL2, SDL2_mixer
# ============================================================================

cmake_minimum_required(VERSION 3.16)
project(OpenRhythmEngine VERSION 0.1.0 LANGUAGES CXX)

# ---------- C++ 标准 ----------
# 强制使用 C++17 标准（用于 std::filesystem, std::optional 等特性）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # 禁用编译器扩展，确保代码可移植性

# ---------- 输出目录 ----------
# 将所有可执行文件输出到 build/bin/ 目录下
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---------- 第三方依赖 ----------
# SDL2: 窗口创建、事件处理、渲染
find_package(SDL2 REQUIRED)
# SDL2_mixer: 音频播放（MP3/OGG/WAV）
find_package(SDL2_mixer REQUIRED)

# ---------- 子项目 ----------
# Engine 目录包含所有游戏引擎源码
add_subdirectory(Engine)