#include "ResourceManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace Ore {

// ============================================================================
// 构造函数 - 默认构造，资源根目录待 Initialize() 确定
// ============================================================================
ResourceManager::ResourceManager() = default;

// ============================================================================
// 析构函数 - 调用 Shutdown() 清理资源
// ============================================================================
ResourceManager::~ResourceManager() {
    Shutdown();
}

// ============================================================================
// Initialize - 初始化资源管理器
//
// 将资源根目录设置为当前工作目录（即运行可执行文件的目录）。
// 这样所有相对路径（如 "Charts/demo.json"、"Songs/music.mp3"）
// 都会基于可执行文件所在目录来解析。
//
// @return 总是返回 true（当前版本无失败路径）
// ============================================================================
bool ResourceManager::Initialize() {
    // 使用 C++17 std::filesystem 获取当前工作目录的绝对路径
    m_assetRoot = std::filesystem::current_path().string();
    std::cout << "ResourceManager initialized. Asset root: " << m_assetRoot << std::endl;
    return true;
}

// ============================================================================
// Shutdown - 清理资源管理器
// 当前版本无需特殊释放操作（使用 RAII 自动管理）
// ============================================================================
void ResourceManager::Shutdown() {
    // 当前无需手动释放资源
}

// ============================================================================
// ResolvePath - 将相对路径解析为绝对路径
//
// 使用 std::filesystem::path 的 / 运算符拼接路径，
// 自动处理跨平台路径分隔符（Windows 的 \ 和 Linux 的 /）。
//
// @param relativePath: 相对于资源根目录的路径（如 "Charts/demo_4k_easy.json"）
// @return 拼接后的完整绝对路径字符串
//
// 示例:
//   资源根目录 = "D:/RhythmGameProject"
//   输入: "Charts/demo.json"
//   输出: "D:/RhythmGameProject/Charts/demo.json"
// ============================================================================
std::string ResourceManager::ResolvePath(const std::string& relativePath) const {
    namespace fs = std::filesystem;
    fs::path assetRoot(m_assetRoot);
    fs::path full = assetRoot / fs::path(relativePath);
    return full.string();
}

// ============================================================================
// ReadTextFile - 读取整个文本文件内容到字符串
//
// 使用 std::ifstream 打开文件，然后通过 stringstream + rdbuf
// 一次性读取全部内容。这是读取小文本文件（如 JSON 谱面）最高效的方式。
//
// @param path: 文件的完整绝对路径
// @return 文件全部文本内容；若文件不存在或无法打开则返回空字符串
//
// 注意: 不适用于超大文件（>100MB），因为会将整个文件加载到内存。
//       对于谱面 JSON（通常 <1MB），这个方法是完全合适的。
// ============================================================================
std::string ResourceManager::ReadTextFile(const std::string& path) const {
    std::ifstream file(path, std::ios::in);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << std::endl;
        return "";
    }

    // stringstream + rdbuf: 一次性将文件流全部读入字符串
    // 这是 C++ 标准库读取文本文件最简洁高效的方式
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// ============================================================================
// FileExists - 检查指定路径的文件是否存在
//
// 使用 C++17 std::filesystem::exists 进行跨平台文件存在性检查。
//
// @param path: 文件的完整绝对路径
// @return true: 文件存在；false: 文件不存在或路径无效
// ============================================================================
bool ResourceManager::FileExists(const std::string& path) const {
    return std::filesystem::exists(path);
}

} // namespace Ore
