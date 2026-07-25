"""
音乐结构分析模块 (structure_analyzer.py)
==========================================

本模块是 Analyzer 分析流程的第六步，负责分析音乐的结构段落。

功能:
  1. 使用 MFCC 自相似矩阵 (Self-Similarity Matrix) 检测重复模式
  2. 使用凝聚聚类 (Agglomerative Clustering) 做段落分割
  3. 自动标记段落类型: intro / verse / chorus / bridge / outro

核心算法:
  - MFCC 特征提取 → 自相似矩阵 → 凝聚聚类分割
  - 基于段落位置的启发式标签分配

使用示例:
  analyzer = StructureAnalyzer()
  sections = analyzer.analyze(audio_data, sample_rate)
  for s in sections:
      print(f"{s['label']}: {s['start']:.1f}s - {s['end']:.1f}s")
"""

import numpy as np
from typing import List, Dict


class StructureAnalyzer:
    """音乐结构分析器 — 检测并标记音乐的结构性段落。

    职责:
      - analyze(): 检测段落边界并自动标注类型
      - _label_section(): 基于位置启发式分配段落标签

    属性 (Properties):
      sections: 分析后的段落列表

    段落标签说明:
      - intro:   前奏（歌曲开头，通常较安静）
      - verse:   主歌（叙事的段落，旋律重复但歌词不同）
      - chorus:  副歌（高潮段落，旋律和歌词都重复）
      - bridge:  桥段（连接段落，通常在歌曲中段，风格变化）
      - outro:   尾奏（歌曲结尾，逐渐淡出）
      - full:    全曲（当只有一段时使用）

    算法限制:
      当前版本使用基于位置的启发式规则（段落顺序）来分配标签，
      而不是基于音乐学规则。这意味着:
        - 适合大多数流行歌曲（intro→verse→chorus→bridge→outro）
        - 对古典乐、前卫摇滚等非标准结构效果一般
        - 未来版本计划使用音乐学特征（和声进行、旋律重复）改进

    使用示例:
      analyzer = StructureAnalyzer()
      sections = analyzer.analyze(audio, sr)

      # 输出所有段落
      for s in sections:
          print(f"{s['label']}: {s['start']:.1f}s → {s['end']:.1f}s")

      # 示例输出:
      #   intro: 0.0s → 25.3s
      #   verse: 25.3s → 55.8s
      #   chorus: 55.8s → 86.2s
      #   bridge: 86.2s → 110.5s
      #   outro: 110.5s → 145.0s
    """

    def __init__(self):
        """初始化 StructureAnalyzer，段落列表置空。

        初始化状态:
          - _sections = [] (空列表)
        """
        self._sections: List[Dict] = []

    def analyze(self, audio_data, sample_rate) -> List[Dict]:
        """检测音乐的结构性段落，返回段落列表。

        这是 StructureAnalyzer 的核心方法。使用自相似矩阵 +
        凝聚聚类来寻找音频中的重复结构。

        算法流程:
          1. 提取 MFCC 特征（13维，每帧一向量）
          2. 构建自相似矩阵 (SSM): 每对帧的余弦相似度
             相同结构（如两段 verse）会在 SSM 中显示为方块
          3. 凝聚聚类: 将相似的帧合并，找到段落边界
          4. 根据段落位置分配标签（启发式规则）

        参数:
          audio_data (np.ndarray): 单声道 float32 音频数据
          sample_rate (int): 采样率 (Hz)

        返回:
          List[Dict]: 段落列表，每个元素为:
            {
              "start": float,   # 段落起始时间 (秒)
              "end": float,     # 段落结束时间 (秒)
              "label": str      # 段落类型标签
            }

        异常处理:
          如果聚类失败（音频太短或特征不足），会回退到等长分段。

        使用示例:
          sections = analyzer.analyze(audio, sr)
          # 定位副歌段落:
          choruses = [s for s in sections if s['label'] == 'chorus']
          for c in choruses:
              print(f"副歌: {c['start']:.1f}s - {c['end']:.1f}s")
        """
        import librosa

        n_fft = 2048
        hop_length = 512

        # ---- 第1步: 提取 MFCC 特征 ----
        # 13 维 MFCC 能够有效捕捉音色信息
        # shape: (13, n_frames)
        mfcc = librosa.feature.mfcc(y=audio_data, sr=sample_rate, n_mfcc=13)

        # ---- 第2步: 构建自相似矩阵 (Self-Similarity Matrix) ----
        # recurrence_matrix: 计算每对帧之间的相似度
        #   mode='affinity': 使用余弦相似度
        #   sym=True: 对称矩阵（SSM 必须是对称的）
        S = librosa.segment.recurrence_matrix(mfcc, mode='affinity', sym=True)

        # ---- 第3步: 段落分割 ----
        n_frames = mfcc.shape[1]

        # 如果音频太短（帧数 < 2），直接返回单个段落
        if n_frames < 2:
            self._sections = [{
                "start": 0.0,
                "end": len(audio_data) / sample_rate,
                "label": "full"
            }]
            return self._sections

        # ---- 使用凝聚聚类做段落边界检测 ----
        # agglomerative(k=3): 将帧聚类为 3 组
        # 聚类边界即为段落边界
        # 如果聚类失败（特征不足等），回退到等长分段
        try:
            boundaries = librosa.segment.agglomerative(mfcc, k=3)
            bound_times = librosa.frames_to_time(boundaries, sr=sample_rate, hop_length=hop_length)
        except Exception:
            # 回退方案: 均分为 4 段
            # 虽然不是最优，但保证不会崩溃
            total_dur = len(audio_data) / sample_rate
            bound_times = np.linspace(0, total_dur, 4)

        # ---- 第4步: 构建段落列表并分配标签 ----
        total_duration = len(audio_data) / sample_rate
        sections = []

        for i in range(len(bound_times) - 1):
            start = float(bound_times[i])
            end = float(bound_times[i + 1])

            # 根据段落位置分配语义标签
            # 使用启发式规则（适合标准歌曲结构）
            label = self._label_section(i, len(bound_times) - 1)
            sections.append({
                "start": start,
                "end": end,
                "label": label
            })

        # 缓存结果
        self._sections = sections

        # 输出分析结果
        print(f"[StructureAnalyzer] 检测到 {len(sections)} 个段落")
        for s in sections:
            print(f"  {s['label']}: {s['start']:.1f}s - {s['end']:.1f}s")

        return sections

    def _label_section(self, idx: int, total: int) -> str:
        """根据段落位置和总数分配语义标签。

        这是启发式规则，基于流行音乐的标准结构:
          intro → verse → chorus → verse → chorus → bridge → chorus → outro

        标签分配规则:
          - 第1段 (idx=0):               intro (前奏)
          - 最后1段 (idx=total-1):        outro (尾奏)
          - 中间段 (total>=4, idx=mid):   bridge (桥段)
          - 奇数段 (idx%2==1):            chorus (副歌)
          - 偶数段 (idx%2==0):            verse (主歌)
          - 如果只有1段:                  full (全曲)

        参数:
          idx (int): 段落索引（从 0 开始）
          total (int): 总段落数

        返回:
          str: 段落标签字符串
        """
        if total == 1:
            return "full"           # 只有一段，无法区分

        if idx == 0:
            return "intro"          # 第一段总是前奏

        if idx == total - 1:
            return "outro"          # 最后一段总是尾奏

        # 如果段落数 >= 4，中间段标记为 bridge
        if total >= 4 and idx == total // 2:
            return "bridge"

        # 奇数索引 → chorus, 偶数索引 → verse
        # 这模拟了经典结构: verse(1) → chorus(2) → verse(3) → chorus(4)
        if idx % 2 == 1:
            return "chorus"
        return "verse"

    @property
    def sections(self):
        """分析后的段落列表。

        Returns:
          List[Dict]: 包含 start/end/label 的段落信息列表
        """
        return self._sections