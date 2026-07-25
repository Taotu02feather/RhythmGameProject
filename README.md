# Open Rhythm Engine (ORE) — 开放式节奏游戏引擎

Open Rhythm Engine 是一个可扩展、数据驱动的节奏游戏引擎，类似于 osu! 和 StepMania。它由两部分组成：**C++ 实时游戏引擎** 和 **Python 离线音乐分析系统**，两者通过标准的 JSON 文件格式进行通信，完全解耦。

## 核心设计理念

1. **用户可自由导入音乐**：只需把 MP3/WAV 文件放入 Songs 目录，运行分析器就能自动生成谱面
2. **音乐自动分析**：检测 BPM、节拍、鼓点、频谱特征和音乐结构
3. **自动生成谱面**：分析结果自动映射为可玩的游戏谱面，支持 2K-8K 轨道和多个难度等级
4. **数据完全分离**：游戏程序、音乐文件、谱面数据、分析结果各自独立，方便扩展和分享

## 架构总览

```
┌──────────────────────────────────────────────────┐
│                 Open Rhythm Engine                │
├────────────────────┬─────────────────────────────┤
│   C++ 游戏引擎      │   Python 音乐分析器           │
│   （实时运行）       │   （离线运行）                │
├────────────────────┼─────────────────────────────┤
│ • SDL2 窗口系统     │ • 音频加载（librosa）          │
│ • 游戏主循环        │ • BPM 检测                    │
│ • 输入管理          │ • 节拍跟踪                    │
│ • 音频播放          │ • 鼓点检测                    │
│ • 硬件加速渲染       │ • 频谱特征提取                │
│ • 谱面加载与解析     │ • 音乐结构分析                │
│ • 判定与计分系统     │ • 自动谱面生成                │
│ • 资源管理          │ • AI 模型接口（预留）          │
├────────────────────┴─────────────────────────────┤
│           通信方式：标准 JSON 文件                   │
│     analysis.json  ←→  chart.json                │
└──────────────────────────────────────────────────┘
```

## 项目结构

```
RhythmGameProject/
├── Engine/                       # C++ 游戏引擎（基于 SDL2）
│   ├── CMakeLists.txt
│   └── src/
│       ├── Core/                 # 引擎核心、游戏循环、渲染器、输入
│       ├── Audio/                # 音频系统（SDL2_mixer）
│       ├── Chart/                # 谱面类型定义与 JSON 加载器
│       ├── Gameplay/             # 判定系统（打击检测、计分）
│       └── Resource/             # 资源管理器（文件 I/O）
│
├── Analyzer/                     # Python 音乐分析系统
│   ├── __init__.py               # 包描述与版本信息
│   ├── analyzer_core/            # 核心分析模块
│   │   ├── __init__.py           # 模块导出
│   │   ├── audio_loader.py       # 音频加载与预处理
│   │   ├── bpm_detector.py       # BPM（歌曲速度）检测
│   │   ├── beat_tracker.py       # 节拍与重拍跟踪
│   │   ├── onset_detector.py     # 音符起始点（鼓点）检测
│   │   ├── feature_extractor.py  # 频谱特征提取
│   │   ├── structure_analyzer.py # 音乐结构分析
│   │   └── chart_generator.py    # 自动谱面生成
│   └── models/                   # AI 模型接口（未来扩展）
│
├── Songs/                        # 用户音乐文件存放目录
├── Charts/                       # 谱面 JSON 文件存放目录
├── Assets/                       # 游戏资源（字体、贴图、音效）
│
├── main.cpp                      # C++ 游戏程序入口
├── run_analyzer.py               # Python 分析器命令行入口
├── CMakeLists.txt                # 顶层 CMake 构建配置
├── requirements.txt              # Python 依赖列表
├── build.bat                     # Windows 一键构建脚本
├── .gitignore
├── LICENSE
└── README.md
```

## 谱面 JSON 格式规范

本项目的谱面采用 **数据驱动** 设计，`lane_count` 是动态字段，支持 2K-8K 模式。

```json
{
  "metadata": {
    "title": "歌曲标题",
    "artist": "歌手名称",
    "charter": "谱面制作者",
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

**Note 类型说明：**

| 类型    | 含义                                             |
| ------- | ------------------------------------------------ |
| `Tap`   | 单次点击 note（最常见的类型）                      |
| `Hold`  | 长按 note，`duration` 为按住时长（秒）             |
| `Slide` | 滑条 note（预留，未来版本实现）                    |

**关键设计细节：**
- `lane_count` 动态配置（1-8），不硬编码轨道数量
- `notes` 数组按 `timestamp` 升序排列
- 所有时间单位均为秒（不使用拍数）
- 可通过 note 的 `extras` 字段自由扩展属性

---

## 环境要求与安装

### C++ 引擎

| 依赖项         | 最低版本    | 用途               |
| -------------- | ----------- | ------------------ |
| CMake          | 3.16+       | 构建系统           |
| SDL2           | 2.x         | 窗口、输入、渲染   |
| SDL2_mixer     | 2.x         | 音频播放           |
| C++ 编译器     | C++17       | MSVC 2019+ 或 MinGW-w64 |

**安装 SDL2 开发库：**

推荐使用 [vcpkg](https://github.com/microsoft/vcpkg)：

```bash
vcpkg install sdl2 sdl2-mixer
```

也可手动下载：
1. 从 [SDL Releases](https://github.com/libsdl-org/SDL/releases) 下载 `SDL2-devel-2.x.x-VC.zip`
2. 从 [SDL_mixer Releases](https://github.com/libsdl-org/SDL_mixer/releases) 下载 `SDL2_mixer-devel-2.x.x-VC.zip`
3. 解压到 `External/SDL2/` 和 `External/SDL2_mixer/`

### Python 分析器

| 依赖项   | 最低版本 | 用途                     |
| -------- | -------- | ------------------------ |
| Python   | 3.9+     | 运行环境                 |
| numpy    | -        | 数值计算                 |
| scipy    | -        | 科学计算                 |
| librosa  | -        | 音频分析与音乐信息提取   |
| soundfile| -        | 音频文件读写             |
| audioread| -        | 多格式音频解码           |

---

## 编译与运行

### 第一步：编译 C++ 游戏引擎

**前置条件：** 已安装 CMake 3.16+ 和 SDL2/SDL2_mixer 开发库。

**方法一：使用一键构建脚本（Windows）**

```bash
build.bat
```

脚本会自动检测系统中的 Visual Studio（2022/2019）或 MinGW，然后配置并编译。

**方法二：手动编译**

```bash
# 1. 创建构建目录
mkdir build
cd build

# 2. 配置 CMake（以 Visual Studio 2022 为例）
cmake .. -G "Visual Studio 17 2022" -A x64

# 3. 编译 Release 版本
cmake --build . --config Release

# 编译产物：bin/OpenRhythmEngine.exe（项目根目录下）
```

**方法三：使用 MinGW（跨平台）**

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

### 第二步：运行 C++ 引擎

```bash
bin\OpenRhythmEngine.exe
```

引擎启动后进入**主菜单**（三页面系统）：

**主菜单页面：**
- ↑↓ 选择菜单项（游玩模式 / 设置 / 退出）
- Enter 确认选择
- ESC 退出程序

**设置页面：**
- ↑↓ 选择要修改的选项（轨道数 / 各轨道键位 / 返回）
- ←→ 切换轨道数（2K-8K，默认 4K）
- Enter 选中某轨道键位后，按下新按键即可修改（支持 A-Z 字母键）
- 已被占用的键位会自动交换
- Backspace 返回主菜单
- **默认键位**：A-S-D-F-G-H-J-K（键盘第三行，A~K 的前 8 个键）

**游玩页面：**
- 加载默认测试谱面 `Charts/demo_4k_easy.json`（16 个 note，4K，120 BPM）
- **键盘输入可视化**：按下对应轨道的按键时，轨道高亮变紫并闪烁
- 终端同步输出按键日志
- ESC 返回主菜单

### 第三步：安装 Python 依赖

```bash
# 创建虚拟环境（推荐）
python -m venv RhythmGamePy

# 激活虚拟环境
# Windows:
RhythmGamePy\Scripts\activate
# macOS / Linux:
source RhythmGamePy/bin/activate

# 安装依赖
pip install -r requirements.txt
```

### 第四步：运行 Python 音乐分析器

```bash
# 基础用法：分析音乐并生成 4K 谱面
python run_analyzer.py Songs/mysong.mp3

# 查看所有可用参数
python run_analyzer.py --help
```

**常用参数：**

| 参数                      | 说明                                               |
| ------------------------- | -------------------------------------------------- |
| `--title "歌名"`          | 设置歌曲标题（默认从文件名推断）                     |
| `--artist "歌手"`         | 设置歌手/作者名                                     |
| `--lanes 6`               | 设置轨道数（2-8，默认 4 = 4K 模式）                 |
| `--difficulty-set`        | 生成 Easy/Normal/Hard 三难度套装                    |
| `--sensitivity 0.4`       | Onset 检测敏感度（0.0=最多，1.0=最少，默认 0.5）     |
| `--analysis-only`         | 仅生成分析数据，跳过谱面生成                         |
| `--output-dir "Charts"`   | 指定输出目录（默认 Charts/）                        |

**完整示例：**

```bash
# 分析一首歌，生成 6K 三难度谱面，指定歌曲信息
python run_analyzer.py Songs/my_song.wav \
    --title "我的精彩歌曲" \
    --artist "我的乐队" \
    --lanes 6 \
    --difficulty-set \
    --sensitivity 0.4
```

**输出文件：**

每次分析会生成两类 JSON 文件：

1. **`<歌名>_analysis.json`** — 完整音乐分析数据，包含：
   - BPM 检测值及置信度
   - 节拍和重拍位置
   - 音符起始点时间和强度
   - 频谱特征（质心、带宽、RMS 能量、过零率）
   - MFCC 和 Chroma 帧级数据（供 AI 训练）
   - 音乐结构段落（intro/verse/chorus/bridge/outro）

2. **`<歌名>_chart.json`** — 可玩的游戏谱面，包含：
   - 完整元数据
   - 动态轨道数量
   - BPM 变化事件
   - Note 数组（时间戳、轨道、类型、时长）

---

## 当前功能清单（v0.1.0）

### C++ 游戏引擎

- [x] SDL2 窗口创建（1280×720，硬件加速）
- [x] 游戏主循环（deltaTime 帧率无关逻辑）
- [x] 输入系统（DFJK 默认布局，支持 8K，边缘/水平双触发模式）
- [x] 硬件加速渲染器（VSync 防止画面撕裂）
- [x] 音频系统（SDL2_mixer，支持 MP3/OGG/WAV）
- [x] 谱面加载器（内置轻量 JSON 解析器，零第三方依赖）
- [x] 数据驱动谱面格式（动态 laneCount，2K-8K）
- [x] 判定系统（Perfect/Great/Good/Miss 四级判定）
- [x] 分数统计（精度计算、连击追踪）
- [x] 资源管理器（路径解析、文件 I/O）
- [x] 演示谱面可视化（动态轨道绘制、判定线）

### Python 音乐分析器

- [x] 多格式音频加载（WAV、MP3、FLAC、OGG）
- [x] BPM 检测与置信度评估
- [x] 节拍和重拍跟踪（4/4 拍假设）
- [x] 音符起始点检测（频谱通量法，多频段支持）
- [x] 频谱特征提取（MFCC、Chroma、质心、带宽、RMS、过零率）
- [x] 音乐结构分析（intro/verse/chorus/bridge/outro 自动标记）
- [x] 自动谱面生成（onset→note 映射，Hold note 自动检测）
- [x] 多难度生成（Easy/Normal/Hard）
- [x] 可配置轨道数（2K-8K）
- [x] AI 模型接口预留（models/ 子包）

---

## 推荐开发路线

### Phase 2：核心玩法（建议下一步）

- [ ] Note 下落渲染系统（falling notes 动画）
- [ ] 基于 BPM 的滚动速度控制
- [ ] 实时输入→判定→计分完整流程
- [ ] 音乐播放与谱面时间同步
- [ ] Combo 显示和精度 UI
- [ ] SDL_ttf 集成（实现文字渲染）

### Phase 3：谱面编辑器

- [ ] 基础 GUI 谱面编辑器
- [ ] Note 放置/删除/编辑工具
- [ ] 音频同步预览播放
- [ ] 保存与加载谱面文件

### Phase 4：高级音乐分析

- [ ] 动态 BPM 变化检测（变速歌曲支持）
- [ ] 多频段 onset→lane 映射优化
- [ ] 模式生成（streams, jumps, chords, stairs）
- [ ] 不同曲风的自适应生成策略

### Phase 5：AI 集成

- [ ] 训练数据收集流程（analysis→chart 配对数据）
- [ ] Audio2Chart seq2seq/Transformer 模型
- [ ] 难度估计模型
- [ ] 跨轨道模式风格迁移（4K→6K/8K 自动转换）

---

## 开发日志

详细的开发记录见 [DEVELOPMENT_LOG.md](DEVELOPMENT_LOG.md)。

---

## 开源协议

非商业使用许可证。源码仅限个人学习、教育及学术研究用途。

Copyright (c) 2026 Taotu02feather