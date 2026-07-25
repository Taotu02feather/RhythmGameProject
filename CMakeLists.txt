# ============================================================================
# Open Rhythm Engine - 顶层 CMake 构建配置
# 
# 功能: 定义项目级构建设置，查找SDL2依赖，引入Engine子目录
# 要求: CMake 3.16+, C++17, SDL2, SDL2_mixer
# ============================================================================

cmake_minimum_required(VERSION 3.16)
project(OpenRhythmEngine VERSION 0.1.0 LANGUAGES CXX)

# ---------- C++ 标准 ----------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------- MSVC 编译选项 ----------
# /utf-8: 源码和执行字符集均为 UTF-8，解决中文注释/字符串乱码
if (MSVC)
    add_compile_options("/utf-8")
endif()

# ---------- 输出目录 ----------
# 将可执行文件输出到项目根目录的 bin/ 目录下（与 README.md 同级）
# 同时覆盖 Debug/Release 子目录配置，确保所有构建类型输出到同一位置
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)

# ---------- 第三方依赖 ----------
# SDL2: 窗口创建、事件处理、渲染
find_package(SDL2 REQUIRED)
# SDL2_mixer: 音频播放（MP3/OGG/WAV）
find_package(SDL2_mixer REQUIRED)

# ---------- 子项目 ----------
# Engine 目录包含所有游戏引擎源码
add_subdirectory(Engine)