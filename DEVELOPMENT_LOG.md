# Open Rhythm Engine — 开发日志

---

## 2026-07-25 — 编译输出优化与键盘输入可视化

### 完成内容

- **CMake 输出目录调整**：将编译产物从 `build/bin/Release/` 改为项目根目录下的 `bin/` 文件夹，运行更方便。
  - 修改 `CMakeLists.txt` 中 `CMAKE_RUNTIME_OUTPUT_DIRECTORY` 为 `${CMAKE_SOURCE_DIR}/bin`
  - 更新 `.gitignore` 忽略 `bin/` 目录
- **ResourceManager 增强**：新增 `GetAssetRoot()` 公开方法，方便外部查询当前资源根目录。
- **main.cpp 大幅增强**：
  - **自动路径检测**：exe 在 `bin/` 子目录下也能自动向上查找项目根目录（通过检测 `Charts/demo_4k_easy.json` 存在性）
  - **默认加载测试谱面**：启动时自动加载 `Charts/demo_4k_easy.json`（16 个 note，4K，120 BPM）
  - **键盘输入可视化**：
    - 按下 D/F/J/K 按键时对应轨道高亮变紫，含顶部/底部发光条
    - 长按时轨道保持高亮状态
    - 按下瞬间有 150ms 闪烁效果
    - 每条轨道下方有按键提示标签（D F J K），按下时标签同步高亮
    - 终端同步输出按键日志 `[Input] 轨道 X (键名) 按下!`
  - **谱面信息展示区域**：右上角显示谱面信息框（Note 数量、BPM、难度）
  - **底部提示栏**：屏幕底部显示操作提示区域
  - **改进的判定线**：双线渲染（金色主判定线 + 橙色半透明阴影）
- **README.md 更新**：同步更新编译输出路径和运行说明，补充键盘可视化功能描述
- **DEVELOPMENT_LOG.md 更新**：补充本条目

### 新增控制器按键（调试用）

| 按键 | 功能 |
|------|------|
| D / F / J / K | 4K 模式轨道 0-3（带可视化反馈） |
| ESC | 退出游戏 |

---

## 2026-07-25 — 代码注释完善与文档更新

### 完成内容

- **C++ 引擎代码注释补充**：为 `Engine/src/Resource/ResourceManager.cpp` 的所有函数补充了详细的汉语注释，包括参数说明、返回值说明、使用示例和内部实现逻辑。
- **Python 分析器代码全面注释**：为 Analyzer 包的全部 10 个 Python 源文件补充了详尽的汉语注释：
  - `Analyzer/__init__.py` — 包级文档（功能概述、架构说明、与 C++ 关系、使用示例）
  - `Analyzer/analyzer_core/__init__.py` — 核心子包模块表、导入说明
  - `analyzer_core/audio_loader.py` — AudioLoader 类的每个方法均有详细的参数/返回/使用示例
  - `analyzer_core/bpm_detector.py` — BPMDetector 的检测算法、置信度计算原理、动态 BPM 原理
  - `analyzer_core/beat_tracker.py` — BeatTracker 的节拍跟踪算法、downbeat 检测方法
  - `analyzer_core/onset_detector.py` — OnsetDetector 的频谱通量法、敏感度含义、多频段检测目的
  - `analyzer_core/feature_extractor.py` — FeatureExtractor 每种特征的含义、MFCC/Chroma 的 AI 训练用途
  - `analyzer_core/structure_analyzer.py` — StructureAnalyzer 的自相似矩阵与凝聚聚类原理、标签分配规则
  - `analyzer_core/chart_generator.py` — ChartGenerator 的 onset→note 映射算法、Hold note 检测、多难度生成策略
  - `Analyzer/models/__init__.py` — AI 模型架构设计说明、训练数据格式、实现优先级
  - `run_analyzer.py` — 含全部 6 步分析流程说明、命令行参数详解
- **README.md 全面重写**：全文改为通顺流畅的汉语，重新组织了内容结构：
  - 添加了核心设计理念章节
  - 添加了环境要求与安装说明表格
  - 补充了详细的四步编译运行流程（引擎编译→引擎运行→Python 依赖→分析器运行）
  - 添加了命令行参数速查表
  - 完善了谱面 JSON 格式说明
- **DEVELOPMENT_LOG.md 更新**：补充了本次所有修改的详细记录。

### 代码注释规范

本次注释遵循了统一的规范：
- 每个源文件开头有模块级文档字符串，说明文件功能、核心算法、使用示例
- 每个类有类级文档字符串，说明职责、属性、使用示例
- 每个公开方法有完整的参数说明、返回值说明、使用示例
- 内部辅助方法有算法原理解释
- 关键算法步骤有行内注释

---

## 2026-07-24 — C++ 引擎 v0.1.0 与 Python 分析器 v0.1.0 完成

### 完成内容

#### C++ 引擎（10 个源文件）

- **Core/Engine.h & Engine.cpp** — 引擎主控类
  - SDL 初始化、窗口创建、子系统生命周期管理
  - 7 步初始化流程（SDL_Init → 子系统创建 → ResourceManager → 窗口 → 渲染器 → 音频 → 按键绑定）
  - Shutdown 按逆序安全释放所有资源
- **Core/GameLoop.h & GameLoop.cpp** — 游戏循环
  - 回调模式注入业务逻辑（onUpdate/onRender）
  - deltaTime 计算（SDL 高精度性能计数器）
  - 帧率防失控保护（deltaTime > 0.1s 强制截断）
- **Core/Renderer.h & Renderer.cpp** — SDL2 渲染器封装
  - 硬件加速渲染 + VSync
  - ClearScreen、DrawRect、DrawText（占位）接口
- **Core/Input.h & Input.cpp** — 输入系统
  - GameAction 枚举支持 8 轨道 + 功能键
  - 边缘触发 vs 水平触发双模式
  - DFJK 默认按键布局
- **Audio/AudioSystem.h & AudioSystem.cpp** — 音频系统
  - SDL2_mixer 封装，Music（流式）+ Chunk（内存）双模式
  - MP3/OGG/WAV 全支持
  - 16 通道音效预分配
- **Chart/ChartTypes.h & ChartTypes.cpp** — 谱面数据结构
  - NoteType 枚举（Tap/Hold/Slide）
  - ChartNote（timestamp/lane/type/duration/extras）
  - Chart（metadata/laneCount/notes/bpmChanges）
- **Chart/ChartLoader.h & ChartLoader.cpp** — 谱面加载器
  - 内置轻量 JSON 解析器（零第三方依赖）
  - SkipWhitespace、ReadJsonString、ReadJsonNumber 三个辅助函数
- **Gameplay/Judge.h & Judge.cpp** — 判定系统
  - 4 级判定（Perfect ±25ms / Great ±50ms / Good ±100ms / Miss ±150ms）
  - ScoreData（计数/连击/精度计算）
- **Resource/ResourceManager.h & ResourceManager.cpp** — 资源管理
  - 路径解析（C++17 std::filesystem）
  - 文本文件读取（stringstream + rdbuf）
- **main.cpp** — 程序入口
  - Engine 初始化 → 加载 demo 谱面 → 设置渲染回调 → 启动游戏循环
  - Demo 模式：根据 laneCount 动态绘制轨道和判定线

#### Python 分析器（10 个模块）

- **audio_loader.py** — AudioLoader：多格式音频加载，统一转换为单声道 float32
- **bpm_detector.py** — BPMDetector：onset 强度 + 自相关算法，全局/动态 BPM 双模式
- **beat_tracker.py** — BeatTracker：已知 BPM 引导的节拍跟踪，4/4 拍 downbeat 标记
- **onset_detector.py** — OnsetDetector：频谱通量法 onset 检测，多频段支持
- **feature_extractor.py** — FeatureExtractor：MFCC(13维)/Chroma/质心/带宽/RMS/ZCR
- **structure_analyzer.py** — StructureAnalyzer：自相似矩阵 + 凝聚聚类，自动标记段落
- **chart_generator.py** — ChartGenerator：onset→note 映射，多难度生成
- **models/__init__.py** — AI 模型接口预留（Audio2Chart/DifficultyEstimator/StyleTransfer）
- **run_analyzer.py** — 命令行入口，argparse 完整参数支持

#### 其他文件

- **CMakeLists.txt** — 顶层 CMake，C++17 标准，Engine 子目录
- **Engine/CMakeLists.txt** — 收集 src/ 下所有文件编译为静态库
- **build.bat** — Windows 一键构建（自动检测 VS2022/VS2019/MinGW）
- **requirements.txt** — numpy, scipy, librosa, soundfile, audioread
- **README.md** — 完整项目中英双语文档
- **PROJECT_CLINE_CONTEXT.md** — 新会话快速上下文恢复文档
- **Charts/demo_4k_easy.json** — 示例 4K 谱面（16 个 note，120 BPM）
- **.gitignore** — 忽略缓存、构建产物、音频文件、AI 模型权重

### 设计决策

1. **数据驱动**：laneCount 是动态字段，Judge 不关心轨道数
2. **JSON 通信解耦**：C++ 和 Python 通过 analysis.json/chart.json 文件通信
3. **零外部 JSON 依赖**：C++ 端自行实现轻量 JSON 解析器
4. **模块化**：每个子系统独立文件对，unique_ptr 管理生命周期
5. **AI 就绪**：Python 分析器输出帧级特征数据

### 技术栈

- **C++ 引擎**：C++17 + SDL2 + SDL2_mixer + CMake
- **Python 分析器**：Python 3.9+ + librosa + numpy + scipy
- **构建系统**：CMake 3.16+
- **版本控制**：Git + GitHub

---

## 2026-07-25 — 项目初始化

### 完成内容

- 创建项目 Git 仓库
- 完成基础项目结构设计
- 添加 README.md 文件
- 添加 LICENSE 文件
- 添加 .gitignore 文件

### 项目目标

本项目旨在开发一个可扩展的节奏游戏引擎，使用户能够导入自己的音乐文件，并通过音乐分析与自动谱面生成技术，将音乐自动转化为可游玩的节奏游戏内容。

### 后续计划

- 搭建基础节奏游戏框架（C++ + SDL2）
- 实现音乐文件读取与分析功能（Python + librosa）
- 探索自动谱面生成方法
- 完善游戏玩法与表现系统
- 规划 AI 深度学习模型集成