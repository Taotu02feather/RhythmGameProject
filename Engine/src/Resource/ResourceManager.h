#pragma once

#include <SDL.h>
#include <string>
#include <unordered_map>
#include <memory>

namespace Ore {

class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();

    bool Initialize();
    void Shutdown();

    // Set the base directory for assets
    void SetAssetRoot(const std::string& path) { m_assetRoot = path; }

    // Resolve a relative path to an absolute path using the asset root
    std::string ResolvePath(const std::string& relativePath) const;

    // Read entire text file
    std::string ReadTextFile(const std::string& path) const;

    // Check if a file exists
    bool FileExists(const std::string& path) const;

private:
    std::string m_assetRoot;
};

} // namespace Ore