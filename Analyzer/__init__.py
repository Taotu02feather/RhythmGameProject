"""
Open Rhythm Engine — 音乐分析器 (Analyzer) 包
===============================================

本包是 Open Rhythm Engine 的离线和 AI 音乐分析系统，负责对用户提供的
音乐文件进行全套自动化分析，并生成可玩节奏游戏谱面。

功能概述:
  - 多格式音频文件加载 (WAV, MP3, FLAC, OGG)
  - BPM (速度) 自动检测，含置信度评估
  - 节拍跟踪与重拍检测 (4/4 拍假设)
  - 音符起始点 (鼓点) 检测，支持多频段分析
  - 频谱特征提取 (MFCC, Chroma, 频谱质心/带宽, RMS, 过零率)
  - 音乐结构分析 (intro/verse/chorus/bridge/outro 自动标记)
  - 自动谱面生成 (onset→note 映射，多难度支持)

与 C++ 引擎的关系:
  本包与 C++ 引擎完全解耦，通过标准 JSON 文件通信:
  - Python 输出 analysis.json (分析数据) → C++ 引擎可读取
  - Python 输出 chart.json (游戏谱面) → C++ 引擎直接加载游玩

AI 就绪设计:
  FeatureExtractor 输出的帧级特征 (_frame_data 字段) 适合作为
  深度学习模型的训练数据。models/ 子包预留了 Audio2Chart 等
  模型的接口架构。

使用方式:
  # 命令行使用
  python run_analyzer.py Songs/mysong.mp3

  # 编程使用
  from Analyzer.analyzer_core import AudioLoader, BPMDetector
  loader = AudioLoader()
  audio, sr = loader.load("Songs/mysong.mp3")
  bpm, conf = BPMDetector().detect(audio, sr)

包版本: 0.1.0
"""

__version__ = "0.1.0"