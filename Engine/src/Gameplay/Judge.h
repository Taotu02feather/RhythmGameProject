#pragma once

#include "Chart/ChartTypes.h"
#include <cstdint>
#include <functional>

namespace Ore {

// Judgment timing windows (configurable)
struct TimingWindow {
    double perfect;  // in seconds (e.g., ±0.025)
    double great;    // in seconds (e.g., ±0.050)
    double good;     // in seconds (e.g., ±0.100)
    double miss;     // if beyond this, it's a miss
};

// Default timing windows for 4K/6K/8K (can be tuned per difficulty)
inline TimingWindow GetDefaultTimingWindow() {
    return { 0.025, 0.050, 0.100, 0.150 };
}

// Judgment result for a single note
enum class Judgment : int {
    Perfect = 0,
    Great,
    Good,
    Miss,

    COUNT
};

inline const char* JudgmentToString(Judgment j) {
    switch (j) {
        case Judgment::Perfect: return "Perfect";
        case Judgment::Great:   return "Great";
        case Judgment::Good:    return "Good";
        case Judgment::Miss:    return "Miss";
        default: return "Unknown";
    }
}

// Score tracking
struct ScoreData {
    int perfectCount = 0;
    int greatCount = 0;
    int goodCount = 0;
    int missCount = 0;
    int totalNotes = 0;
    int maxCombo = 0;
    int currentCombo = 0;

    double CalculateAccuracy() const {
        if (totalNotes == 0) return 0.0;
        // Weighted: Perfect=100%, Great=80%, Good=60%, Miss=0%
        double weighted = perfectCount * 1.0 + greatCount * 0.8 + goodCount * 0.6;
        return weighted / static_cast<double>(totalNotes) * 100.0;
    }

    void RecordHit(Judgment j) {
        switch (j) {
            case Judgment::Perfect: perfectCount++; currentCombo++; break;
            case Judgment::Great:   greatCount++;   currentCombo++; break;
            case Judgment::Good:    goodCount++;    currentCombo++; break;
            case Judgment::Miss:    missCount++;    currentCombo = 0; break;
        }
        if (currentCombo > maxCombo) maxCombo = currentCombo;
    }

    void Reset() {
        perfectCount = greatCount = goodCount = missCount = 0;
        totalNotes = maxCombo = currentCombo = 0;
    }
};

// Judge system - determines hit quality based on timing
// Data-driven: works with any laneCount (no hardcoded lanes)
class Judge {
public:
    Judge();
    ~Judge() = default;

    void SetTimingWindow(const TimingWindow& window) { m_timingWindow = window; }
    const TimingWindow& GetTimingWindow() const { return m_timingWindow; }

    // Judge a hit at a specific lane
    // delta: time difference between player input and note timestamp (positive = late)
    Judgment JudgeHit(double delta) const;

    // Check if a note should be considered missed (player didn't hit in time)
    bool IsMissed(double delta) const;

    // Get the current score data
    const ScoreData& GetScoreData() const { return m_scoreData; }
    ScoreData& GetScoreData() { return m_scoreData; }

    void ResetScore() { m_scoreData.Reset(); }

private:
    TimingWindow m_timingWindow;
    ScoreData m_scoreData;
};

} // namespace Ore