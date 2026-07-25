"""
Analyzer 核心模块 (analyzer_core)

Open Rhythm Engine 的音乐分析核心算法包。

本模块包含 7 个独立子模块，每个子模块负责音乐分析流程中的一个步骤：

  模块                    功能说明
  ---------------------   --------------------------------------------------
  audio_loader.py         AudioLoader — 多格式音频文件加载与预处理
  bpm_detector.py         BPMDetector — 基于 onset 强度 + 自相关的 BPM 检测
  beat_tracker.py         BeatTracker — 在 BPM 指导下精确定位每个节拍
  onset_detector.py       OnsetDetector — 频谱通量法音符起始点（鼓点）检测
  feature_extractor.py    FeatureExtractor — 提取 MFCC/Chroma/频谱质心等特征
  structure_analyzer.py   StructureAnalyzer — 自相似矩阵 + 聚类做音乐段落分割
  chart_generator.py      ChartGenerator — 将分析结果自动转化为可玩游戏谱面

使用方式:
  from Analyzer.analyzer_core import AudioLoader, BPMDetector, ...
  loader = AudioLoader()
  audio, sr = loader.load("Songs/mysong.mp3")
  bpm, conf = BPMDetector().detect(audio, sr)

导出列表:
"""

from .audio_loader import AudioLoader
from .bpm_detector import BPMDetector
from .beat_tracker import BeatTracker
from .onset_detector import OnsetDetector
from .feature_extractor import FeatureExtractor
from .structure_analyzer import StructureAnalyzer
from .chart_generator import ChartGenerator
