"""
音符起始点（鼓点）检测模块 (onset_detector.py)
================================================

本模块是 Analyzer 分析流程的第四步，负责：
  1. 使用频谱通量法 (Spectral Flux) 检测音频中的音符起始位置
  2. 支持多频段检测——区分低频鼓和高频镲
  3. 评估每个 onset 的强度

核心算法:
  - librosa.onset.onset_strength(aggregate=np.median) → onset 强度包络
  - librosa.onset.onset_detect(delta=敏感度) → onset 帧位置
  - librosa.onset.onset_strength_multi(channels=N) → 多频段分解

使用示例:
  detector = OnsetDetector()
  onset_times, onset_strengths = detector.detect(audio, sr, sensitivity=0.5)
  # 多频段检测（区分低频鼓和高频镲）
  band_results = detector.detect_per_band(audio, sr, num_bands=3)
"""

import numpy as np
from typing import Tuple


class OnsetDetector:
    """音符起始点检测器 — 检测音乐中的音符攻击、鼓击、敲击事件。

    职责:
      - detect(): 检测全局 onset 位置和强度
      - detect_per_band(): 多频段 onset 检测（如低频鼓 vs 高频镲分离）

    属性 (Properties):
      onset_times:     onset 发生时间列表 (秒)
      onset_strengths: 每个 onset 的归一化强度 [0, 1]

    什么是 Onset？
      Onset（音符起始点）是音乐中"一个音符开始发声的瞬间"，
      比如鼓手敲击底鼓的瞬间、吉他手拨弦的瞬间、钢琴键被按下
      的瞬间。在节奏游戏中，onset 就是 note 应该被打击的时刻。

    敏感度参数说明:
      - sensitivity 越低 (如 0.1) → 检测更多 onset（更灵敏）
      - sensitivity 越高 (如 0.9) → 仅检测最显著的 onset
      - 默认 0.5 对于大多数流行音乐效果最佳

    使用示例:
      detector = OnsetDetector()

      # 默认敏感度检测
      onset_times, strengths = detector.detect(audio, sr)

      # 低敏感度：捕捉更多细节（适合密集鼓点的音乐）
      onset_times, strengths = detector.detect(audio, sr, sensitivity=0.2)

      # 高敏感度：只保留最强的 onset（适合谱面清理）
      onset_times, strengths = detector.detect(audio, sr, sensitivity=0.8)
    """

    def __init__(self):
        """初始化 OnsetDetector，内部数组置空。

        初始化状态:
          - _onset_times = [] (空数组)
          - _onset_strengths = [] (空数组)
        """
        self._onset_times = np.array([])
        self._onset_strengths = np.array([])

    def detect(self, audio_data, sample_rate, sensitivity=0.5):
        """检测音符起始点，返回 (onset 时间, onset 强度)。

        这是 OnsetDetector 的核心方法。

        算法流程:
          1. 计算 onset 强度包络 (spectral flux 法)
          2. 在包络上挑选峰值位置 (onset_detect)
          3. 根据敏感度调整检测数量
          4. 提取每个 onset 位置的强度值
          5. 强度归一化到 [0, 1]

        参数:
          audio_data (np.ndarray): 单声道 float32 音频数据
          sample_rate (int): 采样率 (Hz)
          sensitivity (float): 检测敏感度，范围 [0.0, 1.0]
            - 越低 = 检测更多 onset = 谱面更密集
            - 越高 = 仅检测显著 onset = 谱面更稀疏
            - 默认 0.5

        返回:
          Tuple[np.ndarray, np.ndarray]:
            - onset_times: onset 的时间 (秒)，从小到大排列
            - onset_strengths: 归一化强度 [0, 1]，与 onset_times 一一对应

        使用示例:
          onset_times, strengths = detector.detect(audio, sr, sensitivity=0.4)
          for t, s in zip(onset_times[:5], strengths[:5]):
              print(f"时间 {t:.3f}s: 强度 {s:.2f}")
        """
        import librosa

        # ---- 第1步: 计算 onset 强度包络 ----
        # aggregate=np.median: 使用中位数聚合各频段的 onset 强度
        #   相比默认的均值聚合，中位数对异常噪声更鲁棒
        onset_env = librosa.onset.onset_strength(
            y=audio_data, sr=sample_rate, hop_length=512,
            aggregate=np.median
        )

        # ---- 第2步: 从包络中检测 onset 帧位置 ----
        # delta: 峰值选择阈值
        #   sensitivity * 0.3: 将用户参数映射到 librosa 的 delta 范围
        # backtrack=True: 回溯找到更精确的 onset 位置
        onset_frames = librosa.onset.onset_detect(
            onset_envelope=onset_env, sr=sample_rate,
            hop_length=512, delta=sensitivity * 0.3, backtrack=True
        )

        # ---- 第3步: 帧索引 → 时间 (秒) ----
        onset_times = librosa.frames_to_time(onset_frames, sr=sample_rate, hop_length=512)

        # ---- 第4步: 提取每个 onset 的强度值 ----
        strengths = []
        for frame in onset_frames:
            if frame < len(onset_env):
                strengths.append(float(onset_env[frame]))
            else:
                strengths.append(0.0)

        onset_strengths = np.array(strengths)

        # ---- 第5步: 强度归一化 ----
        if len(onset_strengths) > 0 and onset_strengths.max() > 0:
            onset_strengths /= onset_strengths.max()

        # 缓存结果
        self._onset_times = onset_times
        self._onset_strengths = onset_strengths

        print(f"[OnsetDetector] 检测到 {len(onset_times)} 个音符起始点")
        return onset_times, onset_strengths

    def detect_per_band(self, audio_data, sample_rate, num_bands=3):
        """多频段 onset 检测，将 onset 按频率分解。

        这个方法将音频分解为多个频段，分别做 onset 检测。
        用途:
          - 低频段 (band 0): 主要检测底鼓 (kick drum)、贝斯
          - 中频段 (band 1): 主要检测军鼓 (snare)、人声
          - 高频段 (band 2): 主要检测镲 (hi-hat/cymbal)

        这个信息未来可用于更智能的 lane 映射:
          - 低频鼓 → 中央轨道
          - 高频镲 → 外围轨道

        参数:
          audio_data (np.ndarray): 单声道 float32 音频数据
          sample_rate (int): 采样率 (Hz)
          num_bands (int): 频段数量，默认 3（低/中/高）

        返回:
          list of Tuple[np.ndarray, np.ndarray]:
            每个频段的 (onset_times, onset_strengths)
            - results[0] = 低频段的 onset
            - results[1] = 中频段的 onset
            - results[2] = 高频段的 onset

        使用示例:
          band_results = detector.detect_per_band(audio, sr, num_bands=3)
          low_onsets, low_strengths = band_results[0]  # 底鼓
          mid_onsets, mid_strengths = band_results[1]  # 军鼓
          high_onsets, high_strengths = band_results[2]  # 镲
        """
        import librosa

        # onset_strength_multi: 多频段 onset 强度
        # channels=num_bands: 将频谱分为 N 个频段
        onset_envs = librosa.onset.onset_strength_multi(
            y=audio_data, sr=sample_rate, hop_length=512, channels=num_bands
        )

        results = []
        for band_idx in range(num_bands):
            # 对每个频段独立检测 onset
            onset_frames = librosa.onset.onset_detect(
                onset_envelope=onset_envs[band_idx], sr=sample_rate,
                hop_length=512, delta=0.15, backtrack=True
            )
            onset_times = librosa.frames_to_time(onset_frames, sr=sample_rate, hop_length=512)

            # 提取强度值
            strengths = []
            for frame in onset_frames:
                if frame < len(onset_envs[band_idx]):
                    strengths.append(float(onset_envs[band_idx][frame]))
                else:
                    strengths.append(0.0)

            onset_strengths = np.array(strengths)
            if len(onset_strengths) > 0 and onset_strengths.max() > 0:
                onset_strengths /= onset_strengths.max()

            results.append((onset_times, onset_strengths))

        return results

    # ------------------------------------------------------------------
    # Properties — 只读属性
    # ------------------------------------------------------------------

    @property
    def onset_times(self):
        """检测到的 onset 时间列表 (秒)。

        Returns:
          np.ndarray: 一维数组，每个元素是一个 onset 的秒级时间戳
        """
        return self._onset_times

    @property
    def onset_strengths(self):
        """每个 onset 的归一化强度。

        值范围 [0.0, 1.0]，表示该 onset 的打击感有多强。
        强度高的 onset 通常对应更显著的打击事件。

        Returns:
          np.ndarray: 一维数组，与 onset_times 一一对应
        """
        return self._onset_strengths