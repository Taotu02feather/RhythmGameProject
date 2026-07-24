#include "ResourceManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace Ore {

ResourceManager::ResourceManager() = default;

ResourceManager::~ResourceManager() {
    Shutdown();
}

bool ResourceManager::Initialize() {
    // Default asset root is the current working directory
    m_assetRoot = std::filesystem::current_path().string();
    std::cout << "ResourceManager initialized. Asset root: " << m_assetRoot << std::endl;
    return true;
}

void ResourceManager::Shutdown() {
    // Nothing to cleanup for now
}

std::string ResourceManager::ResolvePath(const std::string& relativePath) const {
    namespace fs = std::filesystem;
    fs::path assetRoot(m_assetRoot);
    fs::path full = assetRoot / fs::path(relativePath);
    return full.string();
}

std::string ResourceManager::ReadTextFile(const std::string& path) const {
    std::ifstream file(path, std::ios::in);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool ResourceManager::FileExists(const std::string& path) const {
    return std::filesystem::exists(path);
}

} // namespace Ore