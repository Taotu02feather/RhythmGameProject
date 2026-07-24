#include "ChartLoader.h"
#include "Resource/ResourceManager.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <cctype>
#include <cmath>

namespace Ore {

ChartLoader::ChartLoader(ResourceManager* resourceManager)
    : m_resourceManager(resourceManager)
{
}

// ============================================================================
// Minimal JSON Parser
// Since we want zero third-party JSON dependencies in the engine,
// we implement a lightweight parser that handles our chart format.
// ============================================================================

void ChartLoader::SkipWhitespace(const std::string& json, size_t& pos) {
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' ||
           json[pos] == '\n' || json[pos] == '\r' || json[pos] == ',')) {
        ++pos;
    }
}

std::string ChartLoader::ReadJsonString(const std::string& json, size_t& pos) {
    SkipWhitespace(json, pos);
    if (pos >= json.size() || json[pos] != '"') return "";
    ++pos; // skip opening quote

    std::string result;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.size()) {
            ++pos;
            switch (json[pos]) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                default: result += json[pos]; break;
            }
        } else {
            result += json[pos];
        }
        ++pos;
    }
    if (pos < json.size()) ++pos; // skip closing quote
    return result;
}

double ChartLoader::ReadJsonNumber(const std::string& json, size_t& pos) {
    SkipWhitespace(json, pos);
    std::string numStr;
    while (pos < json.size() && (std::isdigit(json[pos]) || json[pos] == '-' ||
           json[pos] == '.' || json[pos] == 'e' || json[pos] == 'E' || json[pos] == '+')) {
        numStr += json[pos];
        ++pos;
    }
    if (numStr.empty()) return 0.0;
    return std::stod(numStr);
}

// ============================================================================
// JSON Serialization
// ============================================================================

static void writeIndent(std::ostream& out, int indent) {
    for (int i = 0; i < indent; ++i) out << "  ";
}

static void writeString(std::ostream& out, const std::string& s) {
    out << '"';
    for (char c : s) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n"; break;
            case '\t': out << "\\t"; break;
            default: out << c; break;
        }
    }
    out << '"';
}

std::string ChartLoader::SerializeToJson(const Chart& chart) const {
    std::ostringstream out;
    out << "{\n";

    // Metadata
    out << "  \"metadata\": {\n";
    out << "    \"title\": \"" << chart.metadata.title << "\",\n";
    out << "    \"artist\": \"" << chart.metadata.artist << "\",\n";
    out << "    \"charter\": \"" << chart.metadata.charter << "\",\n";
    out << "    \"difficulty_name\": \"" << chart.metadata.difficultyName << "\",\n";
    out << "    \"difficulty_level\": " << chart.metadata.difficultyLevel << ",\n";
    out << "    \"audio_file\": \"" << chart.metadata.audioFile << "\",\n";
    if (!chart.metadata.analysisFile.empty()) {
        out << "    \"analysis_file\": \"" << chart.metadata.analysisFile << "\",\n";
    }
    out << "    \"preview_start\": " << chart.metadata.previewStart << "\n";
    out << "  },\n";

    // Lane count
    out << "  \"lane_count\": " << chart.laneCount << ",\n";

    // BPM changes
    out << "  \"bpm_changes\": [\n";
    for (size_t i = 0; i < chart.bpmChanges.size(); ++i) {
        const auto& bpm = chart.bpmChanges[i];
        out << "    { \"timestamp\": " << bpm.timestamp << ", \"bpm\": " << bpm.bpm << " }";
        if (i < chart.bpmChanges.size() - 1) out << ",";
        out << "\n";
    }
    out << "  ],\n";

    // Notes
    out << "  \"notes\": [\n";
    for (size_t i = 0; i < chart.notes.size(); ++i) {
        const auto& note = chart.notes[i];
        out << "    {\n";
        out << "      \"timestamp\": " << note.timestamp << ",\n";
        out << "      \"lane\": " << note.lane << ",\n";
        out << "      \"type\": \"" << NoteTypeToString(note.type) << "\"";
        if (note.type == NoteType::Hold || note.duration > 0.0) {
            out << ",\n      \"duration\": " << note.duration;
        }
        out << "\n    }";
        if (i < chart.notes.size() - 1) out << ",";
        out << "\n";
    }
    out << "  ]\n";
    out << "}\n";

    return out.str();
}

// ============================================================================
// JSON Parsing
// ============================================================================

std::unique_ptr<Chart> ChartLoader::ParseFromJson(const std::string& jsonContent) {
    auto chart = std::make_unique<Chart>();
    size_t pos = 0;

    // Minimal JSON parsing for our controlled format
    // Expect: { "metadata": { ... }, "lane_count": N, "bpm_changes": [...], "notes": [...] }

    // Find metadata block
    auto findKey = [&](const std::string& key) -> bool {
        SkipWhitespace(jsonContent, pos);
        size_t search = jsonContent.find("\"" + key + "\"", pos);
        if (search == std::string::npos) return false;
        // Find the ':' after the key
        size_t colon = jsonContent.find(':', search + key.size() + 2);
        if (colon == std::string::npos) return false;
        pos = colon + 1;
        return true;
    };

    // Parse metadata.title
    if (findKey("title")) {
        chart->metadata.title = ReadJsonString(jsonContent, pos);
    }

    // Parse metadata.artist
    if (findKey("artist")) {
        chart->metadata.artist = ReadJsonString(jsonContent, pos);
    }

    // Parse metadata.charter
    if (findKey("charter")) {
        chart->metadata.charter = ReadJsonString(jsonContent, pos);
    }

    // Parse metadata.difficulty_name
    if (findKey("difficulty_name")) {
        chart->metadata.difficultyName = ReadJsonString(jsonContent, pos);
    }

    // Parse metadata.difficulty_level
    if (findKey("difficulty_level")) {
        chart->metadata.difficultyLevel = static_cast<int>(ReadJsonNumber(jsonContent, pos));
    }

    // Parse metadata.audio_file
    if (findKey("audio_file")) {
        chart->metadata.audioFile = ReadJsonString(jsonContent, pos);
    }

    // Parse metadata.analysis_file (optional)
    if (findKey("analysis_file")) {
        chart->metadata.analysisFile = ReadJsonString(jsonContent, pos);
    }

    // Parse metadata.preview_start
    if (findKey("preview_start")) {
        chart->metadata.previewStart = ReadJsonNumber(jsonContent, pos);
    }

    // Parse lane_count
    if (findKey("lane_count")) {
        chart->laneCount = static_cast<int>(ReadJsonNumber(jsonContent, pos));
        if (chart->laneCount < 1) chart->laneCount = 1;
        if (chart->laneCount > 8) chart->laneCount = 8;
    }

    // Parse BPM changes array
    if (findKey("bpm_changes")) {
        SkipWhitespace(jsonContent, pos);
        if (pos < jsonContent.size() && jsonContent[pos] == '[') {
            ++pos;
            while (pos < jsonContent.size()) {
                SkipWhitespace(jsonContent, pos);
                if (pos >= jsonContent.size() || jsonContent[pos] == ']') break;

                if (jsonContent[pos] == '{') {
                    ++pos;
                    Chart::BPMInfo bpm;
                    if (findKey("timestamp")) {
                        bpm.timestamp = ReadJsonNumber(jsonContent, pos);
                    }
                    if (findKey("bpm")) {
                        bpm.bpm = ReadJsonNumber(jsonContent, pos);
                    }
                    // Skip to end of object
                    size_t close = jsonContent.find('}', pos);
                    if (close != std::string::npos) pos = close + 1;
                    chart->bpmChanges.push_back(bpm);
                } else {
                    ++pos;
                }
            }
        }
    }

    // If no BPM changes, add a default
    if (chart->bpmChanges.empty()) {
        chart->bpmChanges.push_back({0.0, 120.0});
    }

    // Parse notes array
    if (findKey("notes")) {
        SkipWhitespace(jsonContent, pos);
        if (pos < jsonContent.size() && jsonContent[pos] == '[') {
            ++pos;
            while (pos < jsonContent.size()) {
                SkipWhitespace(jsonContent, pos);
                if (pos >= jsonContent.size() || jsonContent[pos] == ']') break;

                if (jsonContent[pos] == '{') {
                    ++pos;
                    ChartNote note;

                    if (findKey("timestamp")) {
                        note.timestamp = ReadJsonNumber(jsonContent, pos);
                    }
                    if (findKey("lane")) {
                        note.lane = static_cast<int>(ReadJsonNumber(jsonContent, pos));
                    }
                    if (findKey("type")) {
                        note.type = StringToNoteType(ReadJsonString(jsonContent, pos));
                    }
                    if (findKey("duration")) {
                        note.duration = ReadJsonNumber(jsonContent, pos);
                    }

                    chart->notes.push_back(note);

                    // Skip to end of object
                    size_t close = jsonContent.find('}', pos);
                    if (close != std::string::npos) pos = close + 1;
                } else {
                    ++pos;
                }
            }
        }
    }

    chart->SortNotes();
    return chart;
}

std::unique_ptr<Chart> ChartLoader::LoadChart(const std::string& chartPath) {
    std::string fullPath = m_resourceManager->ResolvePath(chartPath);
    std::string jsonContent = m_resourceManager->ReadTextFile(fullPath);

    if (jsonContent.empty()) {
        std::cerr << "ChartLoader: No content in chart file: " << fullPath << std::endl;
        return nullptr;
    }

    auto chart = ParseFromJson(jsonContent);
    if (chart) {
        std::cout << "Chart loaded: " << chart->metadata.title
                  << " (" << chart->notes.size() << " notes, "
                  << chart->laneCount << " lanes)" << std::endl;
    }
    return chart;
}

bool ChartLoader::SaveChart(const Chart& chart, const std::string& chartPath) {
    std::string fullPath = m_resourceManager->ResolvePath(chartPath);
    std::string json = SerializeToJson(chart);

    std::ofstream file(fullPath, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "ChartLoader: Failed to save chart to: " << fullPath << std::endl;
        return false;
    }

    file << json;
    file.close();
    std::cout << "Chart saved to: " << fullPath << std::endl;
    return true;
}

} // namespace Ore