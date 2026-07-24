# Open Rhythm Engine (ORE)

An extensible, data-driven rhythm game engine with AI-powered music analysis and automatic chart generation.

开放式、数据驱动的节奏游戏引擎，支持 AI 音乐分析和自动谱面生成。

---

## Architecture | 架构

```
┌──────────────────────────────────────────────────┐
│                  Open Rhythm Engine                │
├────────────────────┬─────────────────────────────┤
│   C++ Game Engine  │   Python Music Analyzer      │
│   (Real-time)      │   (Offline / AI)             │
├────────────────────┼─────────────────────────────┤
│ • SDL2 Window      │ • Audio Loading (librosa)    │
│ • Game Loop        │ • BPM Detection              │
│ • Input System     │ • Beat Tracking              │
│ • Audio Playback   │ • Onset (Drum) Detection     │
│ • Renderer         │ • Feature Extraction         │
│ • Chart Loader     │ • Structure Analysis         │
│ • Judge (Hit)      │ • Chart Generation           │
│ • Resource Mgr     │ • AI Model Interface (future)│
├────────────────────┴─────────────────────────────┤
│          Communication: JSON Files                │
│     analysis.json  ←→  chart.json                │
└──────────────────────────────────────────────────┘
```

**C++ and Python communicate only through standard JSON files** — no direct coupling.
C++ 和 Python **仅通过标准 JSON 文件通信**，无直接耦合。

---

## Project Structure | 项目结构

```
RhythmGameProject/
├── Engine/                    # C++ Game Engine (SDL2)
│   ├── CMakeLists.txt
│   └── src/
│       ├── Core/              # Engine, GameLoop, Renderer, Input
│       ├── Audio/             # AudioSystem (SDL2_mixer)
│       ├── Chart/             # ChartTypes, ChartLoader (JSON parser)
│       ├── Gameplay/          # Judge (hit detection, scoring)
│       └── Resource/          # ResourceManager (file I/O)
│
├── Analyzer/                  # Python Music Analyzer
│   ├── __init__.py
│   ├── analyzer_core/         # Core analysis modules
│   │   ├── audio_loader.py       # Audio file loading
│   │   ├── bpm_detector.py       # BPM/tempo detection
│   │   ├── beat_tracker.py       # Beat & downbeat tracking
│   │   ├── onset_detector.py     # Drum hit detection
│   │   ├── feature_extractor.py  # Spectral feature extraction
│   │   ├── structure_analyzer.py # Music structure analysis
│   │   └── chart_generator.py    # Auto chart generation
│   └── models/                # AI models (future Audio2Chart, etc.)
│
├── Songs/                     # User music files (WAV, MP3, FLAC, OGG)
├── Charts/                    # Chart files (*.json)
├── Assets/                    # Game assets (fonts, textures, sounds)
│   └── fonts/
├── External/                  # Third-party libraries
│
├── main.cpp                   # Game entry point
├── run_analyzer.py            # Analyzer CLI entry point
├── CMakeLists.txt             # Top-level CMake
├── requirements.txt           # Python dependencies
├── build.bat                  # Windows build helper
├── .gitignore
├── LICENSE
└── README.md
```

---

## Chart Format | 谱面格式

Charts are **data-driven** with dynamic `lane_count`. Supports 2K-8K modes.

```json
{
  "metadata": {
    "title": "Song Title",
    "artist": "Artist Name",
    "charter": "Charter Name",
    "difficulty_name": "Easy",
    "difficulty_level": 1,
    "audio_file": "../Songs/song.wav",
    "analysis_file": "../Songs/song_analysis.json",
    "preview_start": 0.0
  },
  "lane_count": 4,
  "bpm_changes": [
    { "timestamp": 0.0, "bpm": 120.0 }
  ],
  "notes": [
    { "timestamp": 1.0, "lane": 0, "type": "Tap" },
    { "timestamp": 2.0, "lane": 1, "type": "Tap" },
    { "timestamp": 3.0, "lane": 2, "type": "Hold", "duration": 0.5 },
    { "timestamp": 4.0, "lane": 3, "type": "Tap" }
  ]
}
```

**Note types:**
- `Tap` — Single press note
- `Hold` — Hold note with `duration` in seconds
- `Slide` — Reserved for future

**Key design decisions:**
- `lane_count` is dynamic (2-8), not hardcoded
- `notes` array sorted by `timestamp`
- All timing in seconds (not beats)
- Extensible via `extras` field in notes

---

## Building the C++ Engine | 编译 C++ 引擎

### Prerequisites

- **CMake 3.16+**
- **SDL2** and **SDL2_mixer** development libraries
- **C++17** compiler (MSVC 2019/2022 or MinGW-w64)

### Install SDL2 (choose one)

**Option A — vcpkg (recommended):**
```bash
vcpkg install sdl2 sdl2-mixer
```

**Option B — Manual download:**
1. Download `SDL2-devel-2.x.x-VC.zip` and `SDL2_mixer-devel-2.x.x-VC.zip` from [SDL Releases](https://github.com/libsdl-org/SDL/releases)
2. Extract to `External/SDL2/` and `External/SDL2_mixer/`

### Build

**Windows (Visual Studio):**
```bash
build.bat
```
Or manually:
```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

**Output:** `build/bin/Release/OpenRhythmEngine.exe`

### Run
```bash
cd build/bin/Release
OpenRhythmEngine.exe
```

The engine will:
1. Create a 1280×720 window titled "Open Rhythm Engine v0.1.0"
2. Load `Charts/demo_4k_easy.json` (sample chart included)
3. Display 4 lanes with the judgment line
4. Press **ESC** to quit

---

## Running the Python Analyzer | 运行 Python 分析器

### Prerequisites

- **Python 3.9+**
- Install dependencies:
```bash
pip install -r requirements.txt
```

### Usage

**Basic analysis and chart generation:**
```bash
python run_analyzer.py Songs/mysong.mp3
```

This will:
1. Load the audio file
2. Detect BPM (tempo)
3. Track beat positions and downbeats
4. Detect onset events (drum hits)
5. Extract spectral features (MFCC, chroma, etc.)
6. Analyze musical structure (intro/verse/chorus)
7. Generate `Charts/mysong_analysis.json` and `Charts/mysong_chart.json`

**Command-line options:**
```bash
python run_analyzer.py <audio_file> [options]

Options:
  --title TITLE          Song title (default: from filename)
  --artist ARTIST        Artist name (default: "Unknown Artist")
  --lanes {2-8}          Number of lanes (default: 4)
  --difficulty-name NAME Difficulty label (default: "Easy")
  --difficulty-level N   Difficulty rating 1-10 (default: 1)
  --difficulty-set       Generate Easy/Normal/Hard set
  --output-dir DIR       Output directory (default: Charts/)
  --analysis-only        Only generate analysis, skip chart
  --sensitivity FLOAT    Onset sensitivity 0.0-1.0 (default: 0.5)
```

**Example with full options:**
```bash
python run_analyzer.py Songs/my_song.wav \
    --title "My Awesome Song" \
    --artist "My Band" \
    --lanes 6 \
    --difficulty-set \
    --sensitivity 0.4
```

### Output Files

1. **`<song>_analysis.json`** — Complete analysis data:
   - BPM with confidence
   - Beat positions and downbeats
   - Onset times and strengths
   - Spectral features (centroid, bandwidth, RMS, ZCR)
   - MFCC and chroma data (for future AI training)
   - Musical structure sections

2. **`<song>_chart.json`** — Playable chart file:
   - Full metadata
   - Dynamic lane count
   - BPM change events
   - Note array (timestamp, lane, type, duration)

---

## Current Capabilities | 当前功能

### C++ Engine (v0.1.0)
- [x] SDL2 window creation (1280×720)
- [x] Game loop with delta time
- [x] Input system with key binding (DFJK default, 8K supported)
- [x] Renderer (SDL2 accelerated, VSync)
- [x] Audio system (SDL2_mixer, MP3/OGG/WAV)
- [x] Chart loader with built-in JSON parser
- [x] Data-driven chart format (2K-8K)
- [x] Judge system with timing windows (Perfect/Great/Good/Miss)
- [x] Score tracking (accuracy, combo)
- [x] Resource manager with path resolution
- [x] Demo chart visualization (lane guides)

### Python Analyzer (v0.1.0)
- [x] Multi-format audio loading (WAV, MP3, FLAC, OGG)
- [x] BPM detection with confidence scoring
- [x] Beat and downbeat tracking
- [x] Onset (drum hit) detection with multi-band analysis
- [x] Spectral feature extraction (MFCC, chroma, centroid, etc.)
- [x] Music structure analysis (intro/verse/chorus/bridge/outro)
- [x] Automatic chart generation (onset→note mapping)
- [x] Multi-difficulty generation (Easy/Normal/Hard)
- [x] Configurable lane count (2K-8K)
- [x] AI model interface placeholder (models/ package)

---

## Design Principles | 设计原则

1. **Separation of Concerns** — Game, music, and charts are completely independent files
2. **Data-Driven** — No hardcoded lane counts, note types, or track configurations
3. **JSON Communication** — C++ and Python communicate only through standard file formats
4. **Modular Architecture** — Each subsystem is an independent module
5. **Extensible** — Chart format supports custom `extras` fields; note types are extensible
6. **AI-Ready** — Python analyzer outputs structured features suitable for ML training

---

## Next Steps | 推荐开发路线

### Phase 2: Core Gameplay (建议下一步)
- [ ] Note rendering system (falling notes animation)
- [ ] Scroll speed control based on BPM
- [ ] Real-time input judgment (connect Input → Judge → Score)
- [ ] Music playback synchronized with chart
- [ ] Combo display and accuracy UI
- [ ] SDL_ttf integration for text rendering

### Phase 3: Chart Editor
- [ ] Basic GUI chart editor (SDL2 or Qt)
- [ ] Note placement/deletion tools
- [ ] Playback preview with audio sync
- [ ] Save/load chart files

### Phase 4: Advanced Analysis
- [ ] Dynamic BPM change detection
- [ ] Multi-band onset-to-lane mapping
- [ ] Pattern generation (streams, jumps, chords)
- [ ] Genre-specific chart generation strategies

### Phase 5: AI Integration
- [ ] Training data collection (analysis→chart pairs)
- [ ] Audio2Chart seq2seq model
- [ ] Difficulty estimation model
- [ ] Style transfer between lane modes

---

## License | 开源协议

Non-Commercial License. Free for personal, educational, and academic use.
非商业使用许可证。源码可用于个人学习、教育及学术研究。

---

## Development Log | 开发日志

See [DEVELOPMENT_LOG.md](DEVELOPMENT_LOG.md)