#pragma once

#include <SDL.h>
#include <string>
#include <unordered_map>
#include <memory>

namespace Ore {

// ============================================================================
// ResourceManager - 资源/文件管理器
//
// 职责:
//   1. 管理资源根目录路径（默认可执行文件所在目录）
//   2. 提供相对路径→绝对路径的解析
//   3. 文件读取和存在性检查
//
// 设计目的: 将文件系统操作集中管理，
//   方便未来切换到虚拟文件系统或资源打包（如 zip/PAK）
// ============================================================================
class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();

    // ---------- 生命周期 ----------

    // Initialize - 初始化资源管理器，默认可执行文件所在目录为根目录
    bool Initialize();

    // Shutdown - 清理资源（当前无特殊操作）
    void Shutdown();

    // ---------- 路径管理 ----------

    // SetAssetRoot - 手动设置资源根目录
    // @param path: 新的根目录路径
    void SetAssetRoot(const std::string& path) { m_assetRoot = path; }

    // ResolvePath - 将相对路径解析为绝对路径
    // @param relativePath: 相对于资源根目录的路径（如 "Charts/demo.json"）
    // @return 完整的绝对路径
    std::string ResolvePath(const std::string& relativePath) const;

    // ---------- 文件 I/O ----------

    // ReadTextFile - 读取整个文本文件内容
    // @param path: 文件路径（绝对路径）
    // @return 文件内容字符串，失败返回空字符串
    std::string ReadTextFile(const std::string& path) const;

    // FileExists - 检查文件是否存在
    // @param path: 文件路径（绝对路径）
    bool FileExists(const std::string& path) const;

    // GetAssetRoot - 获取资源根目录路径
    // @return 当前资源根目录的绝对路径
    const std::string& GetAssetRoot() const { return m_assetRoot; }

private:
    std::string m_assetRoot;  // 资源根目录的绝对路径
};

} // namespace Ore
