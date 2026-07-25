"""
频谱特征提取模块 (feature_extractor.py)
=========================================

本模块是 Analyzer 分析流程的第五步，负责提取音频的各种频谱和时序特征。

提取的特征（供分析器和未来 AI 模型使用）:
  - MFCC (13维): 梅尔频率倒谱系数——音频分类/识别最常用的特征
  - Chroma: 12音高类别——用于和弦和旋律分析
  - 频谱质心 (Spectral Centroid): 频谱的"重心"——反映音色亮度
  - 频谱带宽 (Spectral Bandwidth): 频谱的"宽度"——反映音色复杂度
  - 频谱滚降 (Spectral Rolloff): 累积能量达到 85% 的频率点
  - RMS 能量: 响度/音量估计
  - 过零率 (Zero-Crossing Rate): 信号符号变化率——与音高相关

设计意图:
  这些特征被设计为 AI 深度学习模型的输入（Audio2Chart 等）。
  _frame_data 字段包含每帧原始数据，供模型训练使用。
  聚合特征 (mean/std) 供快速查询和可视化。

使用示例:
  extractor = FeatureExtractor()
  features = extractor.extract(audio_data, sample_rate)
  print(f"频谱质心均值: {features['spectral_centroid_mean']:.0f} Hz")
"""

import numpy as np
from typing import Dict


class FeatureExtractor:
    """频谱特征提取器 — 从音频信号中提取多种声学和音乐特征。

    职责:
      - extract(): 执行全部特征提取，返回字典格式结果
      - 同时输出聚合特征（均值/标准差）和帧级原始数据

    属性 (Properties):
      features: 提取结果字典（与 extract() 返回值相同）

    特征列表:
      聚合特征（标量，用于统计）:
        spectral_centroid_mean/std  — 频谱质心
        spectral_bandwidth_mean/std — 频谱带宽
        spectral_rolloff_mean       — 频谱滚降
        rms_energy_mean/std         — RMS 能量
        zero_crossing_rate_mean/std — 过零率

      帧级数据（二维数组，用于 AI 训练）:
        _frame_data.mfcc               — 13 维 MFCC
        _frame_data.chroma             — 12 维色度
        _frame_data.spectral_centroid  — 每帧频谱质心
        _frame_data.rms                — 每帧 RMS 能量

    使用示例:
      extractor = FeatureExtractor()

      # 提取特征
      features = extractor.extract(audio, sr)

      # 使用聚合特征
      print(f"亮度: {features['spectral_centroid_mean']:.0f} Hz")
      print(f"能量: {features['rms_energy_mean']:.3f}")

      # 使用帧级数据（供 AI 训练）
      mfcc_data = features['_frame_data']['mfcc']  # shape: (13, n_frames)
      chroma_data = features['_frame_data']['chroma']  # shape: (12, n_frames)
    """

    def __init__(self):
        """初始化 FeatureExtractor，特征字典置空。

        初始化状态:
          - _features = {} (空字典，等待 extract() 填充)
        """
        self._features = {}

    def extract(self, audio_data, sample_rate) -> Dict:
        """提取所有音频特征，返回字典格式。

        这是 FeatureExtractor 的核心方法。一次调用即可提取
        所有需要的频谱和时序特征。

        处理流程:
          1. STFT (短时傅里叶变换): 将时域音频转为时频谱
          2. 计算频谱特征: 质心、带宽、滚降
          3. 计算能量特征: RMS (均方根能量)
          4. 计算节奏特征: 过零率 (ZCR)
          5. 谐波/打击分离 (HPSS)
          6. 计算 MFCC (13维) — AI 最重要的特征
          7. 计算 Chroma (12维) — 和弦/旋律分析
          8. 聚合为统计量 (mean/std)

        参数:
          audio_data (np.ndarray): 单声道 float32 音频数据
          sample_rate (int): 采样率 (Hz)

        返回:
          Dict: 特征字典，包含:
            - 聚合统计特征 (9 个标量)
            - _frame_data: 帧级原始数据 (4 个二维数组)

        使用示例:
          features = extractor.extract(audio, sr)
          # features 可直接序列化为 JSON 的 analysis 结果
        """
        import librosa

        n_fft = 2048       # FFT 窗口大小: 2048 个采样点
        hop_length = 512   # 帧移: 512 个采样点 (约 11.6ms @ 44100Hz)

        # ---- 第1步: STFT 频谱计算 ----
        # 将时域信号转为频域表示（时频谱）
        # stft shape: (n_freq_bins, n_frames)
        # n_freq_bins = n_fft/2 + 1 = 1025
        stft = np.abs(librosa.stft(audio_data, n_fft=n_fft, hop_length=hop_length))

        # ---- 第2步: 频谱特征 ----
        # 频谱质心: 频谱的"重心"频率，反映音色亮度
        #   高质心 = 明亮音色（镲、小提琴高音）
        #   低质心 = 低沉音色（贝斯、大鼓）
        centroid = librosa.feature.spectral_centroid(
            S=stft, sr=sample_rate, n_fft=n_fft, hop_length=hop_length
        )

        # 频谱带宽: 频谱能量的"散布范围"，反映音色复杂度
        #   高带宽 = 复杂音色（噪音、镲）
        #   低带宽 = 简单音色（纯音、正弦波）
        bandwidth = librosa.feature.spectral_bandwidth(
            S=stft, sr=sample_rate, n_fft=n_fft, hop_length=hop_length
        )

        # 频谱滚降: 累积能量达到 85% 的频率点
        #   用于区分有调声（低滚降）和噪音（高滚降）
        spectral_rolloff = librosa.feature.spectral_rolloff(
            S=stft, sr=sample_rate, n_fft=n_fft, hop_length=hop_length
        )

        # ---- 第3步: 能量/响度特征 ----
        # RMS (Root Mean Square): 均方根能量，反映响度
        #   高 RMS = 响亮的片段（副歌、高潮）
        #   低 RMS = 安静的片段（前奏、间奏）
        rms = librosa.feature.rms(S=stft)

        # ---- 第4步: 节奏特征 ----
        # 过零率: 信号符号变化的频率
        #   高过零率 = 高频成分多 / 噪音
        #   低过零率 = 低频成分多 / 纯音
        zcr = librosa.feature.zero_crossing_rate(audio_data, hop_length=hop_length)

        # ---- 第5步: 谐波/打击分离 ----
        # HPSS (Harmonic-Percussive Source Separation)
        #   harmonic: 持续的声音（旋律、和声）
        #   percussive: 瞬态的声音（鼓、敲击）
        # 注: 当前版本提取了但未直接使用，为未来分析预留
        harmonic, percussive = librosa.effects.hpss(audio_data)

        # ---- 第6步: MFCC (梅尔频率倒谱系数) ----
        # MFCC 是音频分类/识别最重要、最常用的特征
        # 13 维 = 足够表示音色信息，同时维度不太高
        # 广泛用于: 音乐分类、语音识别、AI 模型
        # shape: (13, n_frames)
        mfcc = librosa.feature.mfcc(y=audio_data, sr=sample_rate, n_mfcc=13)

        # ---- 第7步: Chroma (色度特征) ----
        # 12 维 = 12 个半音 (C, C#, D, ..., B)
        # 反映音乐的和声/和弦信息
        # shape: (12, n_frames)
        chroma = librosa.feature.chroma_stft(
            S=stft, sr=sample_rate, hop_length=hop_length
        )

        # ---- 第8步: 聚合为统计量 ----
        # 将帧级特征聚合为标量统计值（均值 + 标准差）
        # 这样方便快速查询，也避免输出过大的 JSON
        features = {
            "spectral_centroid_mean": float(np.mean(centroid)),
            "spectral_centroid_std": float(np.std(centroid)),
            "spectral_bandwidth_mean": float(np.mean(bandwidth)),
            "spectral_bandwidth_std": float(np.std(bandwidth)),
            "spectral_rolloff_mean": float(np.mean(spectral_rolloff)),
            "rms_energy_mean": float(np.mean(rms)),
            "rms_energy_std": float(np.std(rms)),
            "zero_crossing_rate_mean": float(np.mean(zcr)),
            "zero_crossing_rate_std": float(np.std(zcr)),
        }

        # ---- 保存帧级原始数据（供 AI 模型训练） ----
        # 以 _ 开头表示这是内部数据，序列化时可能需要特殊处理
        # 这些二维数组较大，在存储 analysis.json 时可能被省略
        features["_frame_data"] = {
            "mfcc": mfcc.tolist(),                 # (13, n_frames) → list
            "chroma": chroma.tolist(),              # (12, n_frames) → list
            "spectral_centroid": centroid[0].tolist(),  # (n_frames,) → list
            "rms": rms[0].tolist(),                  # (n_frames,) → list
        }

        self._features = features
        print(f"[FeatureExtractor] 提取了 {len(features) - 1} 个聚合特征")
        return features

    @property
    def features(self):
        """提取的特征字典。

        Returns:
          Dict: 与 extract() 返回值相同的特征字典
        """
        return self._features