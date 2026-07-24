#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

namespace Ore {

// ============================================================================
// NoteType - Note 类型枚举（可扩展）
//
// 当前支持的 note 类型:
//   - Tap: 单次点击 note（最常见的类型）
//   - Hold: 长按 note，带 duration（持续时长）
//   - Slide: 滑条 note（预留，未来版本实现）
//
// 设计原则: 使用枚举而非字符串存储类型，
//   在 JSON 序列化时通过 NoteTypeToString/StringToNoteType 转换
// ============================================================================
enum class NoteType : int {
    Tap = 0,   // 单点 note
    Hold = 1,  // 长按 note（需要按住 duration 秒）
    Slide = 2, // 滑条 note（预留，未实现）

    COUNT      // 类型总数
};

// ============================================================================
// NoteTypeToString - 将 NoteType 枚举转换为 JSON 字符串
// @param type: note 类型枚举值
// @return 对应的字符串表示
// ============================================================================
inline const char* NoteTypeToString(NoteType type) {
    switch (type) {
        case NoteType::Tap:  return "Tap";
        case NoteType::Hold: return "Hold";
        case NoteType::Slide: return "Slide";
        default: return "Unknown";
    }
}

// ============================================================================
// StringToNoteType - 将 JSON 字符串转换为 NoteType 枚举
// @param s: 字符串（如 "Tap", "Hold", "Slide"）
// @return 对应的枚举值，未知字符串默认返回 Tap
// ============================================================================
inline NoteType StringToNoteType(const std::string& s) {
    if (s == "Tap")  return NoteType::Tap;
    if (s == "Hold") return NoteType::Hold;
    if (s == "Slide") return NoteType::Slide;
    return NoteType::Tap; // 默认回退
}

// ============================================================================
// ChartNote - 单个 note 的数据结构
//
// 字段说明:
//   - timestamp: 从歌曲开始的绝对时间（秒），例如 1.5 表示歌曲开始后 1.5 秒
//   - lane: 轨道编号（0 到 laneCount-1），数据驱动，不硬编码
//   - type: note 类型（Tap/Hold/Slide）
//   - duration: Hold note 的持续时长（秒），Tap note 时为 0
//   - extras: 扩展元数据（键值对），用于未来功能而无需修改谱面格式
// ============================================================================
struct ChartNote {
    double timestamp = 0.0;    // note 触发时间（秒，从歌曲开始计算）
    int lane = 0;              // 轨道编号（0-based）
    NoteType type = NoteType::Tap;  // note 类型
    double duration = 0.0;     // 持续时长（秒），仅 Hold note 使用

    // 扩展字段：未来可以通过此 map 添加新属性，不破坏已有谱面
    std::unordered_map<std::string, std::string> extras;
};

// ============================================================================
// ChartMetadata - 谱面元数据
//
// 包含谱面的描述信息，如歌曲名、作者、难度等。
// 这些数据在游戏 UI（选歌界面）中使用。
// ============================================================================
struct ChartMetadata {
    std::string title;             // 歌曲标题
    std::string artist;            // 歌手/作者
    std::string charter;           // 谱面制作者
    std::string difficultyName;    // 难度名称（如 "Easy", "Normal", "Hard"）
    int difficultyLevel = 1;       // 难度等级（1-10）
    std::string audioFile;         // 音频文件相对路径
    std::string analysisFile;      // 分析数据文件路径（可选）
    double previewStart = 0.0;     // 预览起始时间（秒），选歌界面播放片段
};

// ============================================================================
// Chart - 完整的谱面数据结构
//
// 这是谱面系统的核心数据结构，包含:
//   - 元数据（标题、作者、难度等）
//   - 轨道配置（动态 laneCount，支持 2K-8K）
//   - Note 列表（按时间排序）
//   - BPM 变化列表（支持变速谱面）
//
// 数据驱动设计: laneCount 是动态字段，
//   Judge 系统和 Input 系统根据 laneCount 动态适配
// ============================================================================
struct Chart {
    ChartMetadata metadata;      // 谱面元数据

    int laneCount = 4;           // 轨道数量（2-8），数据驱动核心字段
    std::vector<ChartNote> notes; // note 列表，按 timestamp 升序排列
    double firstNoteTime = 0.0;  // 第一个 note 的时间（SortNotes 后自动计算）
    double lastNoteTime = 0.0;   // 最后一个 note 的时间

    // ---------- BPM 信息 ----------
    // 支持变速谱面：在不同时间点切换到不同的 BPM
    struct BPMInfo {
        double timestamp = 0.0;  // BPM 变化的起始时间（秒）
        double bpm = 120.0;      // 该时间段的 BPM 值
    };
    std::vector<BPMInfo> bpmChanges;  // BPM 变化列表

    // ---------- 校验方法 ----------

    // IsValid - 校验谱面数据是否有效
    // @return true: laneCount 在 1-8 范围内，且至少有一个 note
    bool IsValid() const {
        return laneCount >= 1 && laneCount <= 8 && !notes.empty();
    }

    // SortNotes - 按 timestamp 排序所有 note
    // 在加载谱面或手动添加 note 后调用
    void SortNotes();
};

} // namespace Ore
