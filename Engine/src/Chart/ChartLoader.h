#pragma once

#include "ChartTypes.h"
#include <string>
#include <memory>

namespace Ore {

class ResourceManager;

class ChartLoader {
public:
    explicit ChartLoader(ResourceManager* resourceManager);
    ~ChartLoader() = default;

    // Load a chart from a .json file path (relative to asset root)
    std::unique_ptr<Chart> LoadChart(const std::string& chartPath);

    // Save chart to a .json file
    bool SaveChart(const Chart& chart, const std::string& chartPath);

    // Load a chart directly from JSON string (for testing)
    std::unique_ptr<Chart> ParseFromJson(const std::string& jsonContent);

    // Serialize a chart to JSON string
    std::string SerializeToJson(const Chart& chart) const;

private:
    ResourceManager* m_resourceManager;

    // Internal JSON parsing helpers (minimal JSON parser, no external dependency)
    std::string ReadJsonString(const std::string& json, size_t& pos);
    double      ReadJsonNumber(const std::string& json, size_t& pos);
    void        SkipWhitespace(const std::string& json, size_t& pos);
};

} // namespace Ore