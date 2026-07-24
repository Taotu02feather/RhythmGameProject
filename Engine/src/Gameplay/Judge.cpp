#include "Judge.h"
#include <cmath>

namespace Ore {

Judge::Judge()
    : m_timingWindow(GetDefaultTimingWindow())
{
}

Judgment Judge::JudgeHit(double delta) const {
    double absDelta = std::abs(delta);

    if (absDelta <= m_timingWindow.perfect) {
        return Judgment::Perfect;
    } else if (absDelta <= m_timingWindow.great) {
        return Judgment::Great;
    } else if (absDelta <= m_timingWindow.good) {
        return Judgment::Good;
    }

    return Judgment::Miss;
}

bool Judge::IsMissed(double delta) const {
    // A note is missed if the player hasn't hit it and we've passed the miss window
    return delta > m_timingWindow.miss;
}

} // namespace Ore