#pragma once

#include "Chart/ChartTypes.h"
#include <cstdint>
#include <functional>

namespace Ore {

// ============================================================================
// TimingWindow - 判定时间窗口配置
//
// 定义每种判定等级的时间容差（秒）。
// 数值越小判定越严格。可以根据难度等级调整。
//
// 例如 Perfect 窗口 ±0.025 秒：
//   玩家在 note 时间 ±25ms 内按下 → Perfect
//   玩家在 ±50ms 内按下 → Great
//   玩家在 ±100ms 内按下 → Good
//   超出 150ms → Miss
// ============================================================================
struct TimingWindow {
    double perfect;  // Perfect 判定窗口（秒），默认 ±0.025
    double great;    // Great 判定窗口（秒），默认 ±0.050
    double good;     // Good 判定窗口（秒），默认 ±0.100
    double miss;     // 超过此时限判定为 Miss（秒），默认 ±0.150
};

// ============================================================================
// GetDefaultTimingWindow - 获取默认判定窗口
// 适用于 4K/6K/8K 通用模式，未来可根据难度独立配置
// ============================================================================
inline TimingWindow GetDefaultTimingWindow() {
    return { 0.025, 0.050, 0.100, 0.150 };
}

// ============================================================================
// Judgment - 单次打击判定结果枚举
// ============================================================================
enum class Judgment : int {
    Perfect = 0,  // 完美：误差在 perfect 窗口内
    Great,        // 优秀：误差在 great 窗口内
    Good,         // 良好：误差在 good 窗口内
    Miss,         // 失误：误差超出 miss 窗口或未按键

    COUNT
};

// ============================================================================
// JudgmentToString - 将判定结果转换为显示字符串
// ============================================================================
inline const char* JudgmentToString(Judgment j) {
    switch (j) {
        case Judgment::Perfect: return "Perfect";
        case Judgment::Great:   return "Great";
        case Judgment::Good:    return "Good";
        case Judgment::Miss:    return "Miss";
        default: return "Unknown";
    }
}

// ============================================================================
// ScoreData - 分数和统计数据结构
//
// 追踪玩家的打击表现，包括：
//   - 各级判定的计数
//   - 当前连击数和最大连击数
//   - 加权精度计算
// ============================================================================
struct ScoreData {
    int perfectCount = 0;    // Perfect 判定次数
    int greatCount = 0;      // Great 判定次数
    int goodCount = 0;       // Good 判定次数
    int missCount = 0;       // Miss 次数
    int totalNotes = 0;      // note 总数
    int maxCombo = 0;        // 最高连击数
    int currentCombo = 0;    // 当前连击数

    // CalculateAccuracy - 计算加权精度（百分比）
    // 权重: Perfect=100%, Great=80%, Good=60%, Miss=0%
    // @return 精度百分比（0.0 - 100.0）
    double CalculateAccuracy() const {
        if (totalNotes == 0) return 0.0;
        double weighted = perfectCount * 1.0 + greatCount * 0.8 + goodCount * 0.6;
        return weighted / static_cast<double>(totalNotes) * 100.0;
    }

    // RecordHit - 记录一次打击判定结果
    // 自动更新计数和连击状态
    // @param j: 判定结果
    void RecordHit(Judgment j) {
        switch (j) {
            case Judgment::Perfect: perfectCount++; currentCombo++; break;
            case Judgment::Great:   greatCount++;   currentCombo++; break;
            case Judgment::Good:    goodCount++;    currentCombo++; break;
            case Judgment::Miss:    missCount++;    currentCombo = 0; break;  // Miss 打断连击
        }
        if (currentCombo > maxCombo) maxCombo = currentCombo;  // 更新最大连击
    }

    // Reset - 重置所有统计数据
    void Reset() {
        perfectCount = greatCount = goodCount = missCount = 0;
        totalNotes = maxCombo = currentCombo = 0;
    }
};

// ============================================================================
// Judge - 判定系统
//
// 职责:
//   1. 根据玩家操作时间与 note 时间的偏差，判定打击质量
//   2. 管理判定时间窗口（支持不同难度/模式的定制）
//   3. 维护分数统计（ScoreData）
//
// 数据驱动设计: Judge 不关心轨道数量，只关心时间偏差。
//   适用于任意 laneCount（2K-8K），因为判定逻辑与轨道配置无关。
// ============================================================================
class Judge {
public:
    // 构造函数 - 加载默认判定窗口
    Judge();
    ~Judge() = default;

    // ---------- 判定窗口配置 ----------

    // SetTimingWindow - 自定义判定时间窗口
    void SetTimingWindow(const TimingWindow& window) { m_timingWindow = window; }

    // GetTimingWindow - 获取当前判定窗口配置
    const TimingWindow& GetTimingWindow() const { return m_timingWindow; }

    // ---------- 打击判定 ----------

    // JudgeHit - 根据时间偏差判定打击质量
    // @param delta: 玩家按键时间 - note 时间（秒），正数=按下偏晚，负数=按下偏早
    // @return 判定等级
    Judgment JudgeHit(double delta) const;

    // IsMissed - 检查 note 是否应该判定为 Miss
    // 当玩家没按且已经超过 miss 窗口时返回 true
    // @param delta: 当前时间 - note 时间（秒）
    bool IsMissed(double delta) const;

    // ---------- 分数数据 ----------

    // GetScoreData - 获取当前分数统计（只读）
    const ScoreData& GetScoreData() const { return m_scoreData; }

    // GetScoreData - 获取当前分数统计（可修改）
    ScoreData& GetScoreData() { return m_scoreData; }

    // ResetScore - 重置所有分数和连击数据
    void ResetScore() { m_scoreData.Reset(); }

private:
    TimingWindow m_timingWindow;  // 判定时间窗口
    ScoreData m_scoreData;        // 分数统计数据
};

} // namespace Ore
