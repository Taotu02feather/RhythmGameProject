#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

namespace Ore {

// Note types supported by the engine (extensible)
enum class NoteType : int {
    Tap = 0,   // Single press note
    Hold = 1,  // Hold note with duration
    Slide = 2, // Slide note (reserved for future)

    COUNT
};

inline const char* NoteTypeToString(NoteType type) {
    switch (type) {
        case NoteType::Tap:  return "Tap";
        case NoteType::Hold: return "Hold";
        case NoteType::Slide: return "Slide";
        default: return "Unknown";
    }
}

inline NoteType StringToNoteType(const std::string& s) {
    if (s == "Tap")  return NoteType::Tap;
    if (s == "Hold") return NoteType::Hold;
    if (s == "Slide") return NoteType::Slide;
    return NoteType::Tap; // default fallback
}

// A single note in the chart
struct ChartNote {
    double timestamp = 0.0;  // Time in seconds from song start
    int lane = 0;            // Lane index (0 to lane_count - 1)
    NoteType type = NoteType::Tap;
    double duration = 0.0;   // Hold duration in seconds (0 for Tap)

    // Future extensions can be added here without breaking existing charts
    // Additional key-value metadata for future features
    std::unordered_map<std::string, std::string> extras;
};

// Metadata for a chart
struct ChartMetadata {
    std::string title;
    std::string artist;
    std::string charter;          // Who created this chart
    std::string difficultyName;   // e.g. "Easy", "Normal", "Hard", "Expert"
    int difficultyLevel = 1;      // Numeric difficulty rating
    std::string audioFile;        // Relative path to the audio file
    std::string analysisFile;     // Relative path to analysis.json (optional)
    double previewStart = 0.0;    // Preview start time in seconds
};

// The complete chart data structure
struct Chart {
    ChartMetadata metadata;

    int laneCount = 4;                      // Number of lanes (2-8 supported)
    std::vector<ChartNote> notes;           // All notes, sorted by timestamp
    double firstNoteTime = 0.0;             // Time of first note
    double lastNoteTime = 0.0;              // Time of last note

    // BPM information for proper tempo display
    struct BPMInfo {
        double timestamp = 0.0;  // When this BPM starts
        double bpm = 120.0;      // BPM value
    };
    std::vector<BPMInfo> bpmChanges;

    // Validate chart data
    bool IsValid() const {
        return laneCount >= 1 && laneCount <= 8 && !notes.empty();
    }

    // Sort notes by timestamp (call after adding notes)
    void SortNotes();
};

} // namespace Ore