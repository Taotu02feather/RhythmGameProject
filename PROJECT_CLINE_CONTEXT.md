# Open Rhythm Engine — 项目完整上下文文档

> **生成时间:** 2026-07-25  
> **用途:** 在新会话/新设备中快速恢复项目上下文，无需重新分析所有代码。  
> **Git 仓库:** `https://github.com/Taotu02feather/RhythmGameProject.git`  
> **最新 Commit:** `458cf6ce64436b347df34774d9bc094625264213`

---

## 目录

1. [项目定位](#1-项目定位)
2. [完整文件清单与职责](#2-完整文件清单与职责)
3. [架构设计](#3-架构设计)
4. [核心设计决策](#4-核心设计决策)
5. [当前已完成功能](#5-当前已完成功能)
6. [编译与运行方法](#6-编译与运行方法)
7. [谱面 JSON 格式规范](#7-谱面-json-格式规范)
8. [分析结果 JSON 格式规范](#8-分析结果-json-格式规范)
9. [推荐开发路线](#9-推荐开发路线)
10. [新会话启动 Checklist](#10-新会话启动-checklist)

---

## 1. 项目定位

**Open Rhythm Engine (ORE)** 是一个可扩展的节奏游戏引擎，类似 osu!/StepMania。

**核心目标：**
- 用户可以自由导入自己的 mp3/wav 音乐文件
- 系统自动分析音乐（BPM、节拍、鼓点、频谱特征）
- 根据音乐结构自动生成基础游戏谱面
- 用户可通过独立的谱面编辑器手动修改谱面
- 游戏程序、音乐资源、谱面文件、分析数据**完全分离**，方便扩展和分享

**技术架构：**
- **C++ 游戏引擎**（实时）：窗口、游戏循环、输入、音频、渲染、note 显示、判定、资源管理、谱面加载
- **Python 音乐分析系统**（离线）：音频读取、BPM 分析、Beat 检测、鼓点检测、频谱分析、音乐结构分析、自动谱面生成
- **通信方式：** C++ 和 Python 不直接耦合，通过 `analysis.json` 和 `chart.json` 标准文件格式通信

**当前阶段：** 第一阶段（v0.1.0）— 工程雏形，保证编译运行成功。

---

## 2. 完整文件清单与职责

### 2.1 顶层文件

| 文件 | 职责说明 |
|------|---------|
| `CMakeLists.txt` | 顶层 CMake 构建配置。定义 C++17 标准、查找 SDL2/SDL2_mixer、引入 Engine 子目录 |
| `main.cpp` | 游戏入口点。创建 Engine → 初始化 → 加载 demo 谱面 → 设置渲染回调 → 启动游戏循环 |
| `run_analyzer.py` | Python 分析器 CLI 入口。调用 Analyzer 包执行 6 步分析流程，输出 JSON |
| `requirements.txt` | Python 依赖：numpy, scipy, librosa, soundfile, audioread |
| `build.bat` | Windows 一键构建脚本（自动检测 VS2022/VS2019/MinGW） |
| `README.md` | 完整项目文档（中英双语），包含架构说明、编译方法、使用示例、推荐路线 |
| `PROJECT_CONTEXT.md` | 本文件 — 完整上下文文档，供新会话快速理解项目 |
| `DEVELOPMENT_LOG.md` | 开发日志（按日期记录完成内容） |
| `.gitignore` | 忽略 Python 缓存、虚拟环境、CMake 构建产物、音乐文件、AI 模型权重 |
| `LICENSE` | 非商业使用许可证 |

### 2.2 C++ Engine 模块（19 个文件）

#### 构建配置

| 文件 | 职责说明 |
|------|---------|
| `Engine/CMakeLists.txt` | 收集 src/ 下所有 .cpp/.h → 编译 Engine 静态库 → 编译 OpenRhythmEngine.exe |

#### Core 核心模块（8 个文件）

| 文件 | 职责说明 |
|------|---------|
| `Engine/src/Core/Engine.h` | **引擎主控类声明。** 包含 EngineConfig 配置结构体、Engine 类（初始化/运行/关闭）、各子系统访问器。7 步初始化流程有详细中文注释 |
| `Engine/src/Core/Engine.cpp` | **引擎主控类实现。** Initialize() 按顺序：SDL_Init → 创建子系统 → ResourceManager → SDL_CreateWindow → Renderer → AudioSystem → Input。Shutdown() 按逆序释放 |
| `Engine/src/Core/GameLoop.h` | **游戏循环管理器声明。** 使用回调模式（onUpdate/onRender）。管理 deltaTime 和帧计数。有帧率防失控保护 |
| `Engine/src/Core/GameLoop.cpp` | **游戏循环实现。** Run() 每帧执行：计算 deltaTime → 处理 SDL 事件 → 更新输入 → 检查退出 → 调用 onUpdate → 调用 onRender → 交换缓冲区 → 重置输入 |
| `Engine/src/Core/Renderer.h` | **SDL2 渲染器封装声明。** 提供 BeginFrame/EndFrame、ClearScreen、DrawRect、DrawText（占位）接口 |
| `Engine/src/Core/Renderer.cpp` | **SDL2 渲染器实现。** 使用 SDL_RENDERER_ACCELERATED + VSync。DrawText 为占位实现，等待 SDL_ttf 集成 |
| `Engine/src/Core/Input.h` | **输入管理器声明。** 包含 GameAction 枚举（Confirm/Cancel/Pause/Lane0-7）。提供水平触发（IsKeyDown）和边缘触发（IsKeyPressed/Released）两种检测。支持按键绑定 |
| `Engine/src/Core/Input.cpp` | **输入管理器实现。** 默认 DFJK 布局（4K）+ Space/E/I/R 扩展（8K）。ESC=暂停，Enter=确认，Backspace=取消 |

#### Audio 音频模块（2 个文件）

| 文件 | 职责说明 |
|------|---------|
| `Engine/src/Audio/AudioSystem.h` | **音频系统声明。** AudioConfig 配置结构体（采样率44100/立体声/缓冲2048）。Music（流式解码，适合长音频）vs Chunk（全载入内存，适合短音效） |
| `Engine/src/Audio/AudioSystem.cpp` | **音频系统实现。** Initialize() 加载 MP3/OGG 解码器 → 打开音频设备 → 预分配16个音效通道。提供 LoadMusic/PlayMusic/StopMusic/PauseMusic/ResumeMusic 全套接口 |

#### Chart 谱面模块（4 个文件）

| 文件 | 职责说明 |
|------|---------|
| `Engine/src/Chart/ChartTypes.h` | **谱面核心数据结构声明。** 包含 NoteType 枚举（Tap/Hold/Slide）、ChartNote 结构体（timestamp/lane/type/duration/extras）、ChartMetadata（标题/作者/难度/音频路径）、Chart 结构体（laneCount/notes/bpmChanges）。NoteTypeToString 和 StringToNoteType 转换函数 |
| `Engine/src/Chart/ChartTypes.cpp` | **Chart::SortNotes() 实现。** 按 timestamp 升序排列 note，同时更新 firstNoteTime 和 lastNoteTime |
| `Engine/src/Chart/ChartLoader.h` | **谱面加载器声明。** 内置轻量 JSON 解析器（零第三方依赖）。提供 LoadChart/SaveChart/ParseFromJson/SerializeToJson |
| `Engine/src/Chart/ChartLoader.cpp` | **谱面加载器实现。** 包含 3 个 JSON 辅助函数：SkipWhitespace（跳过空白和逗号）、ReadJsonString（读字符串含转义处理）、ReadJsonNumber（读数字含科学计数法）。ParseFromJson() 使用 findKey lambda 按 key 查找解析 |

#### Gameplay 玩法模块（2 个文件）

| 文件 | 职责说明 |
|------|---------|
| `Engine/src/Gameplay/Judge.h` | **判定系统声明。** TimingWindow（Perfect±0.025s/Great±0.05s/Good±0.1s/Miss±0.15s）。Judgment 枚举（Perfect/Great/Good/Miss）。ScoreData（计数/连击/精度计算）。Judge 类 |
| `Engine/src/Gameplay/Judge.cpp` | **判定系统实现。** JudgeHit() 用绝对值比较判定窗口。IsMissed() 检测超时 Miss |

#### Resource 资源模块（2 个文件）

| 文件 | 职责说明 |
|------|---------|
| `Engine/src/Resource/ResourceManager.h` | **资源管理器声明。** 管理资产根目录、相对路径→绝对路径解析、文件读取和存在检查 |
| `Engine/src/Resource/ResourceManager.cpp` | **资源管理器实现。** Initialize() 设可执行文件所在目录为根目录。ReadTextFile() 用 stringstream+rdbuf 高效读取 |

### 2.3 Python Analyzer 模块（10 个文件）

#### 包声明

| 文件 | 职责说明 |
|------|---------|
| `Analyzer/__init__.py` | Analyzer 包说明，版本号 0.1.0 |
| `Analyzer/analyzer_core/__init__.py` | 导出所有核心模块类 |

#### 核心分析模块（7 个文件）

| 文件 | 类的职责 |
|------|---------|
| `Analyzer/analyzer_core/audio_loader.py` | **AudioLoader** — 使用 librosa 加载 WAV/MP3/FLAC/OGG，转为单声道 float32。提供 time_to_sample 和 sample_to_time 转换 |
| `Analyzer/analyzer_core/bpm_detector.py` | **BPMDetector** — 基于 onset 强度 + 自相关的 BPM 检测。detect() 返回 (bpm, confidence)。detect_dynamic() 做滑动窗口变速 BPM 检测。内部 _compute_confidence() 用 IBI 偏差评估置信度 |
| `Analyzer/analyzer_core/beat_tracker.py` | **BeatTracker** — 基于已知 BPM 做节拍跟踪。track() 返回 (beat_times, downbeat_times, beat_strengths)。downbeat 每 4 拍一个（4/4 拍假设） |
| `Analyzer/analyzer_core/onset_detector.py` | **OnsetDetector** — 频谱通量法音符起始点检测。detect() 返回 (onset_times, onset_strengths)。detect_per_band() 做多频段检测（区分低频鼓/高频镲） |
| `Analyzer/analyzer_core/feature_extractor.py` | **FeatureExtractor** — 提取 MFCC(13维)、Chroma、频谱质心/带宽/滚降、RMS 能量、过零率。extract() 返回聚合特征和帧级原始数据 |
| `Analyzer/analyzer_core/structure_analyzer.py` | **StructureAnalyzer** — 使用 MFCC 自相似矩阵 + 凝聚聚类做音乐结构分割。自动标记 intro/verse/chorus/bridge/outro |
| `Analyzer/analyzer_core/chart_generator.py` | **ChartGenerator** — 将 onset 映射到 game note。_map_onsets_to_notes() 根据强度分配合适轨道/类型。generate_difficulty_set() 可生成 Easy/Normal/Hard 三难度 |

#### AI 模型预留

| 文件 | 职责说明 |
|------|---------|
| `Analyzer/models/__init__.py` | AI 模型接口占位。规划了 Audio2Chart、DifficultyEstimator、StyleTransfer 模型架构 |

### 2.4 数据文件

| 文件 | 职责说明 |
|------|---------|
| `Charts/demo_4k_easy.json` | 示例 4K 谱面（16 个 note，含 Tap 和 Hold，120 BPM） |
| `Songs/demo_track_analysis.json` | 示例分析数据（10 秒歌曲，120 BPM，含 beats/onsets/features/structure） |

### 2.5 目录结构（空目录）

| 目录 | 用途说明 |
|------|---------|
| `Songs/` | 用户音乐文件存放目录（.gitignore 中已忽略音频文件） |
| `Charts/` | 谱面 JSON 文件存放目录 |
| `Assets/fonts/` | 字体资源目录（等待 SDL_ttf 集成） |
| `External/` | 第三方库（SDL2 开发包等） |
| `Engine/src/Core/` | 核心引擎模块 |
| `Engine/src/Audio/` | 音频模块 |
| `Engine/src/Chart/` | 谱面模块 |
| `Engine/src/Gameplay/` | 玩法/判定模块 |
| `Engine/src/Resource/` | 资源管理模块 |
| `Analyzer/analyzer_core/` | Python 分析核心算法 |
| `Analyzer/models/` | AI 模型（未来） |
| `RhythmGamePy/` | Python 虚拟环境（已安装 numpy） |

---

## 3. 架构设计

```
┌──────────────────────────────────────────────────┐
│                  Open Rhythm Engine                │
├────────────────────┬─────────────────────────────┤
│   C++ Game Engine  │   Python Music Analyzer      │
│   (Real-time)      │   (Offline)                  │
├────────────────────┼─────────────────────────────┤
│ • SDL2 Window      │ • Audio Loading (librosa)    │
│ • Game Loop        │ • BPM Detection              │
│ • Input System     │ • Beat Tracking              │
│ • Audio Playback   │ • Onset Detection            │
│ • Renderer         │ • Feature Extraction         │
│ • Chart Loader     │ • Structure Analysis         │
│ • Judge System     │ • Chart Generation           │
│ • Resource Mgr     │ • AI Interface (reserved)    │
├────────────────────┴─────────────────────────────┤
│          Communication: JSON Files                 │
│     analysis.json  ←→  chart.json                │
└──────────────────────────────────────────────────┘
```

**子系统依赖关系（C++ 引擎）：**
```
Engine
 ├── GameLoop (依赖 Engine)
 ├── Renderer (依赖 SDL_Window)
 ├── Input (独立)
 ├── AudioSystem (独立)
 ├── ResourceManager (独立)
 └── ChartLoader (依赖 ResourceManager)
```

---

## 4. 核心设计决策

### 4.1 数据驱动（Data-Driven）
- **`lane_count` 是动态字段**（1-8），不在代码中硬编码轨道数
- Judge 系统不假设轨道数，只关心时间偏差
- Input 系统的 GameAction 使用 Lane0-Lane7，支持 2K-8K 自由扩展
- 谱面格式通过 `extras` 字段支持未来扩展，不破坏现有文件

### 4.2 JSON 通信解耦
- C++ 和 Python **不直接调用**，通过标准 JSON 文件交换数据
- Python 生成 `analysis.json` → C++ 读取
- Python 生成 `chart.json` → C++ 读取并运行游戏
- 这样两个系统可以独立开发、独立测试、独立部署

### 4.3 零外部 JSON 依赖
- C++ 端 ChartLoader **自己实现了轻量 JSON 解析器**
- 3 个辅助函数：`SkipWhitespace`、`ReadJsonString`、`ReadJsonNumber`
- 足够解析我们定义的 chart JSON 格式
- 避免引入 nlohmann/json 等额外依赖

### 4.4 模块化
- 每个子系统独立一个 .h/.cpp 文件对
- Engine 通过 unique_ptr 管理子系统的生命周期
- GameLoop 通过回调模式注入业务逻辑

### 4.5 AI 就绪
- FeatureExtractor 输出 MFCC、Chroma 等帧级特征（`_frame_data` 字段）
- `Analyzer/models/__init__.py` 预留了 Audio2Chart 等模型的架构说明
- 分析数据包含完整的 beat/onset/feature 信息，可作为训练数据

---

## 5. 当前已完成功能

### C++ 引擎 (v0.1.0)
- [x] SDL2 窗口创建（1280×720，标题"Open Rhythm Engine v0.1.0"）
- [x] 完整的游戏循环（deltaTime 计算、事件处理、渲染分离）
- [x] 输入系统（DFJK 默认布局，8K 支持，边缘/水平触发双模式）
- [x] SDL2 硬件加速渲染器（VSync 防止撕裂）
- [x] SDL2_mixer 音频系统（MP3/OGG/WAV，Music+Chunk 双模式）
- [x] ChartLoader 谱面加载（内置 JSON 解析器）
- [x] 数据驱动谱面格式（动态 laneCount，3 种 note 类型）
- [x] Judge 判定系统（4 级判定 + 精度/连击统计）
- [x] ResourceManager（路径管理 + 文件 I/O）
- [x] Demo 谱面可视化（根据 laneCount 动态绘制轨道 + 判定线）
- [x] 所有代码已添加完整中文注释，说明每个类和函数的职责

### Python 分析器 (v0.1.0)
- [x] 多格式音频加载（WAV/MP3/FLAC/OGG）
- [x] BPM 检测 + 置信度评估 + 动态 BPM 跟踪
- [x] Beat/Downbeat 跟踪（4/4 拍假设）
- [x] Onset 鼓点检测 + 多频段分析
- [x] 频谱特征提取（MFCC/Chroma/Centroid/Bandwidth/RMS/ZCR）
- [x] 音乐结构分析（intro/verse/chorus/bridge/outro 自动标记）
- [x] 自动谱面生成（onset→note 映射 + Hold 检测）
- [x] 多难度生成（Easy 筛选强 onset、Normal 全 onset、Hard 增加密度）
- [x] CLI 接口（run_analyzer.py + argparse 完整参数支持）
- [x] AI 模型接口预留（models/ 包架构设计）

---

## 6. 编译与运行方法

### 6.1 编译 C++ 引擎

**前置条件：**
- CMake 3.16+
- SDL2 和 SDL2_mixer 开发库
- MSVC 2019/2022 或 MinGW-w64

**安装 SDL2（推荐 vcpkg）：**
```bash
vcpkg install sdl2 sdl2-mixer
```

**构建：**
```bash
# 方式一
build.bat

# 方式二（手动）
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

**输出：** `build/bin/Release/OpenRhythmEngine.exe`

**运行：** 在项目根目录执行（因为需加载相对路径 `Charts/demo_4k_easy.json`）

### 6.2 运行 Python 分析器

```bash
pip install -r requirements.txt
python run_analyzer.py Songs/mysong.mp3
```

详细参数见 `README.md` 或 `python run_analyzer.py --help`

---

## 7. 谱面 JSON 格式规范

```json
{
  "metadata": {
    "title": "歌曲标题",
    "artist": "歌手名",
    "charter": "谱面作者",
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
    { "timestamp": 2.0, "lane": 1, "type": "Hold", "duration": 0.5 }
  ]
}
```

**关键字段说明：**
- `lane_count` — 动态轨道数（1-8），BPM 由 `bpm_changes` 数组管理（支持变速）
- `notes[].timestamp` — 绝对时间（秒），从歌曲开始计算
- `notes[].lane` — 轨道编号（0-based）
- `notes[].type` — "Tap" / "Hold" / "Slide"
- `notes[].duration` — Hold note 持续时长（秒），Tap 为 0
- `notes[].extras` — 扩展字段（键值对），未来新增功能时使用

---

## 8. 分析结果 JSON 格式规范

```json
{
  "source_file": "Songs/mysong.wav",
  "analysis_version": "0.1.0",
  "duration_seconds": 180.5,
  "sample_rate": 44100,
  "bpm": 128.0,
  "bpm_confidence": 0.95,
  "beat_positions": [0.0, 0.47, 0.94, ...],
  "downbeats": [0.0, 1.88, 3.75, ...],
  "beat_strengths": [0.9, 0.7, 0.5, ...],
  "onset_times": [0.0, 0.47, 0.71, ...],
  "onset_strengths": [0.8, 0.6, 0.4, ...],
  "spectral_features": {
    "spectral_centroid_mean": 1500.0,
    "spectral_bandwidth_mean": 800.0,
    "rms_energy_mean": 0.3,
    "zero_crossing_rate_mean": 0.15
  },
  "structure": {
    "sections": [
      { "start": 0.0, "end": 30.0, "label": "intro" },
      { "start": 30.0, "end": 90.0, "label": "verse" },
      { "start": 90.0, "end": 150.0, "label": "chorus" }
    ]
  },
  "generated_chart_path": "Charts/mysong_chart.json"
}
```

---

## 9. 推荐开发路线

### Phase 2：核心玩法（建议下一步）
- [ ] Note 下落渲染系统（falling notes 动画）
- [ ] 基于 BPM 的滚动速度控制
- [ ] 实时输入 → Judge → Score 判定流程
- [ ] 音乐播放与谱面同步
- [ ] Combo 显示和精度 UI
- [ ] **SDL_ttf 集成**（替换 DrawText 占位实现，实现文字渲染）

### Phase 3：谱面编辑器
- [ ] 基础 GUI 谱面编辑器
- [ ] Note 放置/删除/编辑工具
- [ ] 音频同步预览播放
- [ ] 保存/加载谱面文件

### Phase 4：高级音乐分析
- [ ] 动态 BPM 变化检测（变速歌曲支持）
- [ ] 多频段 onset→lane 映射优化（低频鼓→内轨，高频镲→外轨）
- [ ] 模式生成（streams, jumps, chords, stairs）
- [ ] 不同曲风的自适应生成策略

### Phase 5：AI 集成
- [ ] 训练数据收集流程（analysis→chart 配对数据集）
- [ ] Audio2Chart seq2seq/Transformer 模型
- [ ] 难度估计模型
- [ ] 跨轨道模式风格迁移（4K→6K 自动转换）

---

## 10. 新会话启动 Checklist

当在新设备或新会话中开始工作时，按以下步骤操作：

### 环境准备
```bash
# 1. 克隆仓库
git clone https://github.com/Taotu02feather/RhythmGameProject.git
cd RhythmGameProject

# 2. 安装 SDL2（Windows 用 vcpkg）
vcpkg install sdl2 sdl2-mixer

# 3. 创建 Python 虚拟环境
python -m venv RhythmGamePy
RhythmGamePy\Scripts\activate   # Windows
# source RhythmGamePy/bin/activate  # macOS/Linux

# 4. 安装 Python 依赖
pip install -r requirements.txt
```

### 理解项目
1. **先读本文件**（PROJECT_CONTEXT.md）了解全局
2. **读 README.md** 了解架构细节
3. **看 Engine/src/Core/Engine.cpp** 了解初始化流程
4. **看 main.cpp** 了解程序入口和渲染循环
5. **看 Chart/demo_4k_easy.json** 了解谱面格式

### 编译运行
```bash
# C++ 引擎
build.bat

# Python 分析器
python run_analyzer.py --help
```

### 关键文件速查

| 想了解的内容 | 查看的文件 |
|-------------|-----------|
| 整体架构 | `PROJECT_CONTEXT.md` + `README.md` |
| C++ 引擎初始化 | `Engine/src/Core/Engine.cpp` |
| 游戏循环 | `Engine/src/Core/GameLoop.cpp` |
| 谱面格式定义 | `Engine/src/Chart/ChartTypes.h` |
| 谱面加载/保存 | `Engine/src/Chart/ChartLoader.cpp` |
| 判定逻辑 | `Engine/src/Gameplay/Judge.cpp` |
| Python BPM 检测 | `Analyzer/analyzer_core/bpm_detector.py` |
| Python 自动谱面生成 | `Analyzer/analyzer_core/chart_generator.py` |
| 完整分析流程 | `run_analyzer.py` |

---

> **最后更新：** 2026-07-25  
> **当前版本：** v0.1.0（工程雏形，可编译运行）  
> **所有文件已添加完整中文注释**