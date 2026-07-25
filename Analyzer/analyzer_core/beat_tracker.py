"""
节拍跟踪与重拍检测模块 (beat_tracker.py)
===========================================

本模块是 Analyzer 分析流程的第三步，负责：
  1. 在已知 BPM 的前提下，精确跟踪每个节拍的时间位置
  2. 识别每小节第一拍——重拍 (downbeat)
  3. 评估每个节拍的力度/强度

核心算法:
  - librosa.onset.onset_strength → 计算 onset 强度包络
  - librosa.beat.beat_track(start_bpm=已知BPM) → 精确节拍跟踪
  - 每 4 拍一个 downbeat (4/4 拍假设)

使用示例:
  tracker = BeatTracker()
  beat_times, downbeat_times, strengths = tracker.track(audio, sr, bpm=120.0)
  print(f"共检测到 {len(beat_times)} 个节拍，{len(downbeat_times)} 个重拍")
"""

import numpy as np
from typing import Tuple


class BeatTracker:
    """节拍跟踪器 — 在 BPM 指导下精确定位每个节拍的时间位置。

    职责:
      - track(): 输入音频和 BPM，输出每个节拍的时间、重拍时间、节拍强度
      - 将节拍信息缓存为属性，供后续 chart_generator 生成谱面使用

    属性 (Properties):
      beat_times:     每个节拍的时间 (秒) numpy 数组
      downbeat_times: 每个重拍 (小节第一拍) 的时间 (秒) numpy 数组
      beat_strengths: 每个节拍的强度 (0.0-1.0) numpy 数组

    关于重拍 (Downbeat):
      在 4/4 拍音乐中，每小节有 4 拍。第 1 拍称为"重拍"(downbeat)，
      通常力度更强。本模块默认每 4 拍标记一个 downbeat。
      这个假设适合绝大多数流行、摇滚、电子音乐。

    使用示例:
      tracker = BeatTracker()

      # 在已知 BPM 的前提下进行节拍跟踪
      beat_times, downbeat_times, beat_strengths = tracker.track(
          audio_data, sample_rate=44100, bpm=128.0
      )

      # beat_times:  [0.00, 0.47, 0.94, 1.41, ...]  (每个节拍的时间)
      # downbeat_times: [0.00, 1.88, 3.75, ...]  (每小节第一拍)
      # beat_strengths: [0.9, 0.7, 0.5, 0.8, ...] (归一化强度)
    """

    def __init__(self):
        """初始化 BeatTracker，所有内部数组初始化为空。

        初始化状态:
          - _beat_times = [] (空数组)
          - _downbeat_times = [] (空数组)
          - _beat_strengths = [] (空数组)
        """
        self._beat_times = np.array([])
        self._downbeat_times = np.array([])
        self._beat_strengths = np.array([])

    def track(self, audio_data, sample_rate, bpm):
        """跟踪节拍位置，返回 (节拍时间, 重拍时间, 节拍强度)。

        这是 BeatTracker 的核心方法。基于已知 BPM 进行精确的节拍跟踪，
        相比 BPMDetector（只需要估计 BPM 值），本方法需要确定每个节拍的
        精确时间位置。

        算法流程:
          1. 计算 onset 强度包络（与 BPMDetector 类似）
          2. 使用 start_bpm 参数告知 beat_track 已知的 BPM
             这样可以避免检测到 1/2 或 2 倍 BPM
          3. 将节拍帧索引转换为时间 (秒)
          4. 从 onset 包络中提取每个节拍的强度
          5. 强度归一化到 [0, 1]
          6. 每 4 拍标记一个 downbeat

        参数:
          audio_data (np.ndarray): 单声道 float32 音频数据
          sample_rate (int): 采样率 (Hz)
          bpm (float): 已知的 BPM 值（来自 BPMDetector.detect()）

        返回:
          Tuple[np.ndarray, np.ndarray, np.ndarray]:
            - beat_times: 每个节拍的时间 (秒)
            - downbeat_times: 每个重拍的时间 (秒)，每 4 拍一个
            - beat_strengths: 每个节拍的归一化强度 [0, 1]

        使用示例:
          bpm, _ = BPMDetector().detect(audio, sr)
          beat_times, downbeats, strengths = BeatTracker().track(audio, sr, bpm)

          # 节拍时间可直接用于谱面生成:
          for t in beat_times:
              chart_generator.place_note(t, lane=...)
        """
        import librosa

        hop_length = 512

        # ---- 第1步: 计算 onset 强度包络 ----
        # onset_strength: 每个分析帧的"变化剧烈程度"
        # 包络值越大的位置越可能是节拍
        onset_env = librosa.onset.onset_strength(
            y=audio_data, sr=sample_rate, hop_length=hop_length
        )

        # ---- 第2步: 在已知 BPM 指导下跟踪节拍 ----
        # start_bpm=bpm: 告诉 librosa 已知的 BPM，
        #   防止检测算法被 1/2 或 2 倍 BPM 误导
        # tightness=100: 严格的节奏跟踪
        tempo, beats = librosa.beat.beat_track(
            onset_envelope=onset_env, sr=sample_rate,
            hop_length=hop_length, start_bpm=bpm, tightness=100
        )

        # ---- 第3步: 将帧索引转换为时间 ----
        # librosa.frames_to_time: 帧索引 → 秒
        beat_times = librosa.frames_to_time(beats, sr=sample_rate, hop_length=hop_length)

        # ---- 第4步: 提取每个节拍的强度 ----
        # 从 onset 包络中取出每个节拍帧对应的值
        strengths = []
        for beat_frame in beats:
            if beat_frame < len(onset_env):
                strengths.append(float(onset_env[beat_frame]))
            else:
                strengths.append(0.0)

        beat_strengths = np.array(strengths)

        # ---- 第5步: 强度归一化 ----
        # 将强度缩放到 [0, 1]，方便后续阈值比较
        if beat_strengths.max() > 0:
            beat_strengths /= beat_strengths.max()

        # ---- 第6步: 标记重拍 (Downbeat) ----
        # 4/4 拍假设: 每 4 拍 = 1 小节
        # 索引 0, 4, 8, 12, ... 是 downbeat
        downbeat_indices = np.arange(0, len(beat_times), 4)
        downbeat_times = beat_times[downbeat_indices]

        # 缓存结果
        self._beat_times = beat_times
        self._downbeat_times = downbeat_times
        self._beat_strengths = beat_strengths

        print(f"[BeatTracker] 找到 {len(beat_times)} 个节拍, {len(downbeat_times)} 个重拍")
        return beat_times, downbeat_times, beat_strengths

    # ------------------------------------------------------------------
    # Properties — 只读属性
    # ------------------------------------------------------------------

    @property
    def beat_times(self):
        """所有节拍的时间位置 (秒)。

        Returns:
          np.ndarray: 一维数组，每个元素是一个节拍的秒级时间戳
        """
        return self._beat_times

    @property
    def downbeat_times(self):
        """所有重拍 (downbeat) 的时间位置 (秒)。

        重拍 = 每小节的第一拍。在 4/4 拍中，每 4 个节拍有 1 个重拍。

        Returns:
          np.ndarray: 一维数组，每个元素是一个重拍的秒级时间戳
        """
        return self._downbeat_times

    @property
    def beat_strengths(self):
        """每个节拍的归一化强度。

        值范围 [0.0, 1.0]，表示该节拍的"打击感"有多强。
        强度高的节拍通常对应鼓点较重的时刻。

        Returns:
          np.ndarray: 一维数组，与 beat_times 一一对应
        """
        return self._beat_strengths