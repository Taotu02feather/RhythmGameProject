"""
BPM（每分钟节拍数 / 歌曲速度）检测模块 (bpm_detector.py)
=========================================================

本模块是 Analyzer 分析流程的第二步，负责：
  1. 使用 librosa 的 onset 强度 + 自相关算法估计歌曲的全局 BPM
  2. 基于节拍一致性计算置信度评分 (0.0-1.0)
  3. 支持滑动窗口的动态 BPM 检测（用于变速歌曲）

核心算法:
  - 主检测: librosa.onset.onset_strength → librosa.beat.beat_track
  - 动态检测: 滑动窗口 + 逐段 librosa.beat.tempo
  - 置信度: IBI (Inter-Beat Interval) 偏差启发式评估

使用示例:
  detector = BPMDetector()
  bpm, confidence = detector.detect(audio_data, 44100)
  print(f"BPM: {bpm:.1f}, 置信度: {confidence:.2f}")
  # 对于变速歌曲:
  dynamic_bpms = detector.detect_dynamic(audio_data, 44100)
"""

import numpy as np
from typing import Tuple, Optional


class BPMDetector:
    """BPM 检测器 — 从音频信号中估计歌曲的主要速度 (BPM)。

    职责:
      - detect(): 检测全局 BPM 并返回置信度
      - detect_dynamic(): 检测随时间变化的 BPM（变速歌曲）
      - _compute_confidence(): 基于节拍间距一致性的置信度启发式算法

    属性 (Properties):
      bpm:        检测到的全局 BPM 值
      confidence: BPM 检测的置信度 (0.0-1.0)

    BPM 常见范围参考:
      - 40-60:  极慢（抒情慢歌、氛围音乐）
      - 60-90:  慢速（民谣、慢摇）
      - 90-120: 中速（流行、摇滚）
      - 120-160: 快速（电子、舞曲）
      - 160-300: 极快（Drum & Bass、Speedcore）

    本检测器将 BPM 限制在 40-300 范围内。

    使用示例:
      detector = BPMDetector()

      # 检测全局 BPM
      bpm, conf = detector.detect(audio, sr)
      print(f"BPM: {bpm:.1f}, 置信度: {conf:.2%}")

      # 检测变速（如某些古典乐或前卫摇滚）
      if conf < 0.5:
          dynamic = detector.detect_dynamic(audio, sr)
          for t, b in dynamic:
              print(f"  {t:.1f}s: BPM = {b:.1f}")
    """

    def __init__(self):
        """初始化 BPMDetector，设置默认值。

        初始化状态:
          - _bpm = 120.0 (默认 BPM，常见流行歌曲速度)
          - _confidence = 0.0 (尚未检测，置信度为 0)
          - _dynamic_tempo = False (默认使用全局 BPM 模式)
        """
        self._bpm: float = 120.0
        self._confidence: float = 0.0
        self._dynamic_tempo: bool = False

    # ------------------------------------------------------------------
    # Public API — 公开接口
    # ------------------------------------------------------------------

    def detect(self, audio_data: np.ndarray, sample_rate: int) -> Tuple[float, float]:
        """检测音频的主要 BPM 并返回 (BPM, 置信度)。

        这是 BPMDetector 的核心方法，使用两级算法:
          1. 计算 Onset 强度包络 (onset_strength) — 反映音乐中的"打击感"
          2. 使用 librosa.beat.beat_track 从 onset 包络推断 BPM

        算法参数说明:
          - hop_length=512: 每帧 512 个采样点（约 11.6ms @ 44100Hz），
            在时间精度和计算效率之间取得平衡
          - tightness=100: 节拍追踪的紧密程度，值越大节奏越严格

        参数:
          audio_data (np.ndarray): 单声道 float32 音频数据，归一化到 [-1, 1]
          sample_rate (int): 采样率 (Hz)，如 44100

        返回:
          Tuple[float, float]: (bpm, confidence)
            - bpm: BPM 值，范围 [40.0, 300.0]
            - confidence: 置信度，范围 [0.0, 1.0]

        使用示例:
          audio, sr = loader.load("song.mp3")
          bpm, conf = BPMDetector().detect(audio, sr)
          if conf > 0.8:
              print("BPM 检测高度可信")
          else:
              print("BPM 检测不确定，可能存在变速")
        """
        import librosa

        # ---- 方法1: 基于 Onset 强度的 BPM 估计 ----
        # onset_strength: 计算每个帧的"打击强度"
        # 类似于检测音乐中"鼓点"或"音符开始"的位置
        onset_env = librosa.onset.onset_strength(
            y=audio_data, sr=sample_rate, hop_length=512
        )

        # beat_track: 从 onset 包络估计节奏和节拍位置
        # tightness=100: 更严格的节奏跟踪（适合电子/流行音乐）
        # 返回值: tempo=BPM估计, beats=节拍帧索引
        tempo, beats = librosa.beat.beat_track(
            onset_envelope=onset_env,
            sr=sample_rate,
            hop_length=512,
            tightness=100
        )

        # librosa 可能返回标量或数组，统一处理
        if isinstance(tempo, np.ndarray):
            bpm = float(tempo[0])
        else:
            bpm = float(tempo)

        # 将 BPM 限制在合理范围内
        # 低于 40: 可能是半速检测（如很慢的歌），强制加倍也合理
        # 高于 300: 可能是倍速检测，限制为 300
        bpm = max(40.0, min(300.0, bpm))

        # ---- 计算置信度 ----
        # 基于节拍间距一致性评估置信度
        confidence = self._compute_confidence(beats, onset_env, bpm, sample_rate)

        # 缓存结果
        self._bpm = bpm
        self._confidence = confidence

        print(f"[BPMDetector] 估计 BPM: {bpm:.1f} (置信度: {confidence:.2f})")
        return bpm, confidence

    def detect_dynamic(
        self, audio_data: np.ndarray, sample_rate: int, window_sec: float = 4.0
    ) -> np.ndarray:
        """检测随音乐时间变化的动态 BPM（用于变速歌曲）。

        使用滑动窗口法: 将音频分成重叠的时段，每段独立估计 BPM。
        适用于古典乐、前卫摇滚等有 BPM 变化的歌曲。

        算法细节:
          - 窗口大小: window_sec（默认 4 秒）
          - 窗口步长: 窗口大小的一半（50% 重叠）
          - 窗口太短 → BPM 估计不稳定
          - 窗口太长 → 无法捕捉快速的速度变化
          - 默认 4 秒对于大多数音乐是很好的平衡

        参数:
          audio_data (np.ndarray): 单声道 float32 音频数据
          sample_rate (int): 采样率 (Hz)
          window_sec (float): 每段窗口时长 (秒)，默认 4.0

        返回:
          np.ndarray: shape (n, 2) 的数组，每行为 (时间, BPM)

        使用示例:
          dynamic = detector.detect_dynamic(audio, sr)
          # dynamic = [[0.0, 120.0], [2.0, 121.5], [4.0, 160.2], ...]
          for t, bpm_val in dynamic:
              print(f"时间 {t:.1f}s: BPM = {bpm_val:.1f}")
        """
        import librosa

        hop_length = 512
        # 将窗口时长转换为帧数
        frame_length = int(window_sec * sample_rate / hop_length)

        # 计算 onset 强度包络
        onset_env = librosa.onset.onset_strength(
            y=audio_data, sr=sample_rate, hop_length=hop_length
        )

        # ---- 滑动窗口 BPM 估计 ----
        tempos = []
        # 每次移动半个窗口长度（50% 重叠），保证平滑过渡
        for i in range(0, len(onset_env), frame_length // 2):
            segment = onset_env[i: i + frame_length]
            # 段太短则停止（避免末段不稳定估计）
            if len(segment) < frame_length // 2:
                break

            # 对当前段进行 BPM 估计
            tempo = librosa.beat.tempo(
                onset_envelope=segment,
                sr=sample_rate,
                hop_length=hop_length
            )

            # 将帧索引转换为时间（秒）
            t = librosa.frames_to_time(i, sr=sample_rate, hop_length=hop_length)
            tempos.append((t, float(tempo[0] if isinstance(tempo, np.ndarray) else tempo)))

        self._dynamic_tempo = True
        return np.array(tempos)

    # ------------------------------------------------------------------
    # Properties — 只读属性
    # ------------------------------------------------------------------

    @property
    def bpm(self) -> float:
        """检测到的全局 BPM 值。

        Returns:
          float: BPM 值（每分钟节拍数），范围 [40.0, 300.0]
        """
        return self._bpm

    @property
    def confidence(self) -> float:
        """BPM 检测的置信度。

        置信度含义:
          0.0-0.3: 低 — 可能检测错误或存在变速
          0.3-0.6: 中 — 基本可靠，但建议验证
          0.6-0.8: 高 — 较为可靠
          0.8-1.0: 很高 — 非常可靠的检测

        Returns:
          float: 置信度，范围 [0.0, 1.0]
        """
        return self._confidence

    # ------------------------------------------------------------------
    # Internal Helpers — 内部辅助方法
    # ------------------------------------------------------------------

    def _compute_confidence(
        self,
        beats: np.ndarray,
        onset_env: np.ndarray,
        bpm: float,
        sample_rate: int
    ) -> float:
        """计算 BPM 估计的启发式置信度评分。

        算法思路:
          1. 计算实际节拍间隔 (Inter-Beat Intervals, IBI)
          2. 计算理想节拍间隔 (60 / BPM)
          3. 统计偏差在 20% 以内的节拍占比
          4. 将该占比缩放为置信度

        为什么用偏差而不是相关系数？
          对于大部分流行音乐，BPM 很稳定，节拍间隔应该均匀。
          如果实际 IBI 和理想 IBI 高度一致 → 置信度高。
          如果 IBI 忽大忽小 → BPM 可能不准或有变速。

        参数:
          beats (np.ndarray): 节拍帧索引数组
          onset_env (np.ndarray): onset 强度包络
          bpm (float): 估计的 BPM 值
          sample_rate (int): 采样率 (Hz)

        返回:
          float: 置信度 [0.0, 1.0]
        """
        import librosa

        # 至少需要 2 个节拍才能计算间隔
        if len(beats) < 2:
            return 0.0

        # 将帧索引转换为时间（秒）
        beat_times = librosa.frames_to_time(beats, sr=sample_rate, hop_length=512)

        if len(beat_times) < 2:
            return 0.0

        # 计算实际节拍间隔 (IBI = Inter-Beat Interval)
        # np.diff: 计算相邻时间差 [t1-t0, t2-t1, t3-t2, ...]
        ibis = np.diff(beat_times)

        # 计算理想节拍间隔
        # 例如 BPM=120: ideal_ibi = 60/120 = 0.5 秒
        ideal_ibi = 60.0 / bpm

        # 计算每个实际间隔与理想间隔的偏差比例
        # 例如实际 0.55s, 理想 0.50s → 偏差 = |0.55-0.50|/0.50 = 0.10 (10%)
        deviations = np.abs(ibis - ideal_ibi) / ideal_ibi

        # 统计偏差在 20% 以内的"好节拍"
        # 例如理想 0.50s，允许 0.40-0.60s
        good_beats = np.sum(deviations < 0.2)
        ratio = good_beats / len(ibis)

        # 缩放至 [0, 1] 并限制上限
        # ratio * 1.2: 给一点奖励（即使只有 83% 的节拍一致也能得到 1.0）
        return min(1.0, ratio * 1.2)