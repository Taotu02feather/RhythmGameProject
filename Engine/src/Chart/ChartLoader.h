#pragma once

#include "ChartTypes.h"
#include <string>
#include <memory>

namespace Ore {

class ResourceManager;

// ============================================================================
// ChartLoader - 谱面加载器（内置轻量 JSON 解析器）
//
// 职责:
//   1. 从 JSON 文件加载谱面 → Chart 数据结构
//   2. 将 Chart 数据结构保存为 JSON 文件
//   3. 提供 JSON 字符串 ↔ Chart 的序列化/反序列化
//
// 设计决策 - 为什么不用 nlohmann/json 等第三方库？
//   - 谱面 JSON 格式是精确可控的（我们定义格式）
//   - 避免额外依赖，降低编译难度
//   - 轻量解析器足够处理谱面格式（metadata / lane_count / bpm_changes / notes）
//   - 未来如果需要完整 JSON 支持，可以替换为 nlohmann/json，接口不变
//
// 文件路径约定:
//   所有路径均为相对于资源根目录的相对路径
//   例如 "Charts/demo_4k_easy.json"
// ============================================================================
class ChartLoader {
public:
    // 构造函数 - 保存资源管理器指针（用于文件路径解析）
    // @param resourceManager: 资源管理器指针（不拥有所有权）
    explicit ChartLoader(ResourceManager* resourceManager);
    ~ChartLoader() = default;

    // ---------- 文件 I/O ----------

    // LoadChart - 从指定路径加载谱面 JSON 文件
    // @param chartPath: 相对于资源根目录的路径
    // @return Chart 智能指针（加载失败返回 nullptr）
    std::unique_ptr<Chart> LoadChart(const std::string& chartPath);

    // SaveChart - 将谱面数据保存到 JSON 文件
    // @param chart: 要保存的谱面数据
    // @param chartPath: 保存路径
    // @return true: 保存成功
    bool SaveChart(const Chart& chart, const std::string& chartPath);

    // ---------- JSON 序列化/反序列化 ----------

    // ParseFromJson - 从 JSON 字符串解析谱面（用于测试或内存加载）
    // @param jsonContent: 完整的 chart JSON 字符串
    // @return Chart 智能指针
    std::unique_ptr<Chart> ParseFromJson(const std::string& jsonContent);

    // SerializeToJson - 将谱面序列化为 JSON 字符串
    // @param chart: 谱面数据
    // @return 格式化的 JSON 字符串（带缩进）
    std::string SerializeToJson(const Chart& chart) const;

private:
    ResourceManager* m_resourceManager;        // 资源管理器（用于路径解析）

    // ---------- 内部 JSON 解析辅助函数 ----------
    // 这些函数实现了一个极简的递归下降 JSON 解析器，
    // 仅够解析我们自己定义的 chart JSON 格式

    // ReadJsonString - 读取双引号包围的 JSON 字符串
    // @param json: 完整 JSON 文本
    // @param pos: 当前解析位置（会被更新）
    // @return 解码后的字符串
    std::string ReadJsonString(const std::string& json, size_t& pos);

    // ReadJsonNumber - 读取 JSON 数字（整数或浮点数）
    // @param json: 完整 JSON 文本
    // @param pos: 当前解析位置（会被更新）
    // @return 数值
    double      ReadJsonNumber(const std::string& json, size_t& pos);

    // SkipWhitespace - 跳过空白字符和逗号
    // @param json: JSON 文本
    // @param pos: 当前解析位置（会被更新）
    void        SkipWhitespace(const std::string& json, size_t& pos);
};

} // namespace Ore
