"""
音频文件加载与预处理模块 (audio_loader.py)
===========================================

本模块是 Analyzer 分析流程的第一步，负责：
  1. 加载多种格式的音频文件 (WAV, MP3, FLAC, OGG)
  2. 将音频统一转换为单声道 float32 数组（归一化到 [-1, 1]）
  3. 提供采样 ↔ 时间的转换函数
  4. 缓存加载后的音频数据，供后续所有分析模块使用

依赖库: librosa（底层使用 soundfile / audioread）

使用示例:
  loader = AudioLoader()
  audio_data, sample_rate = loader.load("Songs/mysong.mp3")
  print(f"时长: {loader.duration:.1f} 秒")
  sample_idx = loader.time_to_sample(1.5)  # 1.5秒处对应的采样索引
"""

import os
import numpy as np
from typing import Tuple, Optional


class AudioLoader:
    """音频加载器 — 加载并预处理音频文件，为所有后续分析提供统一的数据接口。

    职责:
      - 封装 librosa.load()，处理各种音频格式的兼容性问题
      - 统一输出为单声道 float32，归一化到 [-1.0, 1.0]
      - 记录并缓存音频元数据（采样率、时长、文件路径）

    属性 (Properties):
      audio_data:  已加载的原始单声道音频信号 (float32 numpy 数组)
      sample_rate: 音频采样率 (Hz)，默认 44100
      duration:    音频总时长 (秒)
      filepath:    原始音频文件路径

    设计说明:
      本类每次实例化只能加载一个音频文件。如需处理多个音频，
      请为每个文件创建新的 AudioLoader 实例，或复用同一个
      实例（调用 load() 会覆盖之前的数据）。
    """

    def __init__(self):
        """初始化 AudioLoader，设置默认参数。

        初始化后的状态:
          - _sample_rate = 44100 (CD 音质采样率)
          - _audio_data = None (尚未加载任何音频)
          - _duration = 0.0
          - _filepath = "" (空字符串)
        """
        self._sample_rate: int = 44100
        self._audio_data: Optional[np.ndarray] = None
        self._duration: float = 0.0
        self._filepath: str = ""

    # ------------------------------------------------------------------
    # Public API — 公开接口
    # ------------------------------------------------------------------

    def load(self, filepath: str, target_sr: int = 44100) -> Tuple[np.ndarray, int]:
        """加载音频文件并返回 (音频数据, 采样率)。

        这是 AudioLoader 的核心方法，也是 Analyzer 分析流程的入口。
        调用后，所有属性（audio_data, sample_rate, duration, filepath）
        都会被更新。

        处理流程:
          1. 检查文件是否存在（文件不存在直接抛异常）
          2. 使用 librosa.load() 加载音频（自动处理各格式）
          3. 重采样到 target_sr（默认 44100 Hz）
          4. 转换为单声道（立体声自动混合为单声道）
          5. 转换为 float32 类型并缓存

        参数:
          filepath (str): 音频文件的完整路径，如 "Songs/mysong.mp3"
                          支持格式: WAV, MP3, FLAC, OGG
          target_sr (int): 目标采样率 (Hz)，默认 44100
                           所有分析模块基于此采样率工作

        返回:
          Tuple[np.ndarray, int]: (audio_data, sample_rate)
            - audio_data: 单声道 float32 numpy 数组，范围 [-1.0, 1.0]
            - sample_rate: 实际采样率 (Hz)

        异常:
          FileNotFoundError: 当 filepath 指向的文件不存在时抛出

        使用示例:
          loader = AudioLoader()
          audio, sr = loader.load("Songs/my_song.wav")
          # audio 是 numpy 数组，sr 是采样率
          print(f"加载成功: {len(audio)} 个采样点, {sr} Hz")
        """
        import librosa

        # 1. 检查文件是否存在
        if not os.path.exists(filepath):
            raise FileNotFoundError(f"音频文件不存在: {filepath}")

        self._filepath = filepath

        # 2. 使用 librosa 加载音频
        #    - sr=target_sr: 重采样到目标采样率
        #    - mono=True: 多声道自动混合为单声道
        #    librosa 底层使用 soundfile 或 audioread，
        #    自动处理 WAV/MP3/FLAC/OGG 等格式
        audio, sr = librosa.load(filepath, sr=target_sr, mono=True)

        # 3. 转换为 float32 并缓存
        #    float32 在内存和计算效率之间取得良好平衡
        self._audio_data = audio.astype(np.float32)
        self._sample_rate = sr
        self._duration = len(audio) / sr

        # 4. 输出加载信息（便于调试和确认）
        print(f"[AudioLoader] 已加载: {filepath}")
        print(f"  采样率: {sr} Hz")
        print(f"  时长: {self._duration:.2f}s")
        print(f"  采样点数: {len(audio)}")

        return self._audio_data, self._sample_rate

    # ------------------------------------------------------------------
    # Properties — 只读属性
    # ------------------------------------------------------------------

    @property
    def audio_data(self) -> Optional[np.ndarray]:
        """已加载的原始单声道音频信号。

        Returns:
          Optional[np.ndarray]: float32 numpy 数组，范围 [-1.0, 1.0]
                                如果尚未调用 load() 则返回 None
        """
        return self._audio_data

    @property
    def sample_rate(self) -> int:
        """音频的采样率 (Hz)。

        采样率表示每秒的采样点数。常见的值:
          - 44100: CD 音质（默认值）
          - 48000: 专业音频（DVD/视频常用）
          - 22050: 半采样率（节省内存，精度稍低）

        Returns:
          int: 采样率 (Hz)
        """
        return self._sample_rate

    @property
    def duration(self) -> float:
        """音频的总时长 (秒)。

        计算方法: duration = 总采样点数 / 采样率

        Returns:
          float: 时长 (秒)，例如 180.5 表示 3 分 0.5 秒
        """
        return self._duration

    @property
    def filepath(self) -> str:
        """原始音频文件的完整路径。

        Returns:
          str: 文件路径字符串，如果尚未加载则返回空字符串
        """
        return self._filepath

    # ------------------------------------------------------------------
    # Utility Methods — 工具方法
    # ------------------------------------------------------------------

    def time_to_sample(self, time_sec: float) -> int:
        """将时间 (秒) 转换为采样索引。

        转换公式: sample_index = time_sec × sample_rate

        参数:
          time_sec (float): 时间 (秒)，例如 1.5 表示第 1.5 秒

        返回:
          int: 对应的采样点索引（从 0 开始）

        使用示例:
          # 获取第 2.0 秒处的采样值
          idx = loader.time_to_sample(2.0)
          sample_value = loader.audio_data[idx]
        """
        return int(time_sec * self._sample_rate)

    def sample_to_time(self, sample_idx: int) -> float:
        """将采样索引转换为时间 (秒)。

        转换公式: time_sec = sample_index / sample_rate

        参数:
          sample_idx (int): 采样点索引（从 0 开始）

        返回:
          float: 对应的时间 (秒)

        使用示例:
          # 第 44100 个采样点对应的时间
          t = loader.sample_to_time(44100)  # 如果 sr=44100，结果为 1.0
        """
        return sample_idx / self._sample_rate