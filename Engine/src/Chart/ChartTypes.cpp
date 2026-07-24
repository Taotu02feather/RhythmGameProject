#include "ChartTypes.h"
#include <algorithm>

namespace Ore {

void Chart::SortNotes() {
    std::sort(notes.begin(), notes.end(),
        [](const ChartNote& a, const ChartNote& b) {
            return a.timestamp < b.timestamp;
        });

    if (!notes.empty()) {
        firstNoteTime = notes.front().timestamp;
        lastNoteTime = notes.back().timestamp;
    }
}

} // namespace Ore