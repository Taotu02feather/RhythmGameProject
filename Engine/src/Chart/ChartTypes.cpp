#include "ChartTypes.h"
#include <algorithm>

namespace Ore {

// ============================================================================
// SortNotes - 按时间戳升序排列所有 note
//
// 调用时机: 加载谱面后立即调用，确保 note 按时间顺序排列
// 副作用: 同时更新 firstNoteTime 和 lastNoteTime
// ============================================================================
void Chart::SortNotes() {
    // lambda 按 timestamp 升序排序
    std::sort(notes.begin(), notes.end(),
        [](const ChartNote& a, const ChartNote& b) {
            return a.timestamp < b.timestamp;
        });

    // 更新第一个和最后一个 note 的时间
    if (!notes.empty()) {
        firstNoteTime = notes.front().timestamp;   // 最早 note 的时间
        lastNoteTime = notes.back().timestamp;      // 最晚 note 的时间
    }
}

} // namespace Ore
