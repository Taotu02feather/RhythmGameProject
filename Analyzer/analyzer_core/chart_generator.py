"""
自动谱面生成模块 (chart_generator.py)
======================================

本模块是 Analyzer 分析流程的最后一步（可独立），负责将音乐分析结果
自动转化为可游玩的节奏游戏谱面 (Chart)。

功能:
  1. 将 onset (音符起始点) 映射到游戏轨道上的 note
  2. 根据 onset 强度分配轨道和 note 类型 (Tap / Hold)
  3. 支持 2K-8K 轨道模式
  4. 支持单难度和多难度 (Easy/Normal/Hard) 生成
  5. 谱面输出为标准 ORE JSON 格式，可直接被 C++ 引擎加载

核心算法:
  _map_onsets_to_notes(): onset → lane 映射 + Hold note 检测
  _map_beats_to_notes(): 回退方案（基于节拍而不是 onset）
  generate_difficulty_set(): Easy/Normal/Hard 三难度生成

使用示例:
  generator = ChartGenerator()
  chart = generator.generate(
      analysis_result, "我的歌", "乐队名",
      audio_file="../Songs/mysong.mp3",
      lane_count=4, difficulty_name="Normal"
  )
  generator.save("Charts/mysong.json")
"""

import json
import os
import numpy as np
from typing import Dict, List


class ChartGenerator:
    """谱面生成器 — 将音乐分析结果转化为可玩节奏游戏谱面。

    职责:
      - generate(): 根据分析数据生成单个难度谱面
      - generate_difficulty_set(): 批量生成 Easy/Normal/Hard 三难度
      - save(): 将谱面保存为 ORE JSON 格式文件
      - _map_onsets_to_notes(): onset → note 的核心映射算法
      - _map_beats_to_notes(): 回退方案（无 onset 时使用节拍）
      - _merge_chords(): 合并同时间 note 为和弦（预留）

    属性 (Properties):
      chart: 当前生成的谱面字典

    轨道映射策略:
      - 强 onset (strength > 0.7): 映射到中央轨道 → 强调感
      - 弱 onset (strength < 0.7): 按节拍序号分配到各轨道 → 分散
      - 密集 onset (间隔 < beat_dur*1.5): 检测为 Hold note
      - 极弱 onset (strength < 0.15): 丢弃 → 防止谱面过于密集

    使用示例:
      generator = ChartGenerator()

      # 单难度谱面
      chart = generator.generate(
          analysis_result=analysis,
          title="My Song",
          artist="Artist",
          audio_file="../Songs/song.mp3",
          lane_count=4,
          difficulty_name="Normal",
          difficulty_level=3
      )
      generator.save("Charts/my_song.json")

      # 多难度套装
      difficulties = generator.generate_difficulty_set(
          analysis_result=analysis,
          title="My Song",
          artist="Artist",
          audio_file="../Songs/song.mp3",
          lane_count=4
      )
      # 生成 Easy, Normal, Hard 三个谱面
    """

    def __init__(self):
        """初始化 ChartGenerator，谱面数据置空。

        初始化状态:
          - _chart = {} (空字典，等待 generate() 填充)
        """
        self._chart = {}

    def generate(
        self,
        analysis_result: Dict,
        title: str = "Unknown",
        artist: str = "Unknown",
        audio_file: str = "",
        lane_count: int = 4,
        difficulty_name: str = "Easy",
        difficulty_level: int = 1,
    ) -> Dict:
        """根据分析数据生成完整的节奏游戏谱面。

        这是 ChartGenerator 的核心方法。将 Python Analyzer 的
        分析结果（BPM/beats/onsets）转化为 ORE 标准谱面格式。

        处理流程:
          1. 从 analysis_result 提取 BPM、节拍、onset 数据
          2. 如果有 onset → 使用 _map_onsets_to_notes() 映射
          3. 如果无 onset → 使用 _map_beats_to_notes() 回退
          4. 构建 BPM 变化列表
          5. 组装完整的 chart 字典

        参数:
          analysis_result (Dict): Analyzer 的分析结果字典，必须包含:
            - bpm (float): BPM 值
            - beat_positions (list): 节拍时间列表
            - onset_times (list): onset 时间列表
            - onset_strengths (list): onset 强度列表
          title (str): 歌曲标题，默认 "Unknown"
          artist (str): 歌手/作者名，默认 "Unknown"
          audio_file (str): 音频文件相对路径（谱面中引用）
          lane_count (int): 轨道数量 (2-8)，默认 4
          difficulty_name (str): 难度名称，默认 "Easy"
          difficulty_level (int): 难度数字等级 (1-10)，默认 1

        返回:
          Dict: 完整的 chart 字典，ORE 标准格式:
            {
              "metadata": {...},
              "lane_count": 4,
              "bpm_changes": [...],
              "notes": [...]
            }

        使用示例:
          chart = generator.generate(
              analysis_result=analysis,
              title="我的歌",
              artist="歌手",
              audio_file="../Songs/song.mp3",
              lane_count=4,
              difficulty_name="Hard",
              difficulty_level=7
          )
          # chart 可直接保存为 JSON 或被 C++ 引擎加载
        """
        # 提取分析数据，提供默认值
        bpm = analysis_result.get("bpm", 120.0)
        beat_times = np.array(analysis_result.get("beat_positions", []))
        onset_times = np.array(analysis_result.get("onset_times", []))
        onset_strengths = np.array(analysis_result.get("onset_strengths", []))

        notes = []

        # ---- Note 生成策略 ----
        if len(onset_times) > 0 and len(beat_times) > 0:
            # 首选方案: onset → note 映射（检测到的音符起始点）
            notes = self._map_onsets_to_notes(
                onset_times, onset_strengths, beat_times, bpm, lane_count
            )
        elif len(beat_times) > 0:
            # 回退方案: 节拍 → note 映射（无 detectable onset 时）
            # 适用于电子音乐等 onset 检测效果差的曲风
            notes = self._map_beats_to_notes(beat_times, lane_count)

        # ---- 构建 BPM 变化列表 ----
        # 当前版本仅支持全局 BPM（单速）
        # 未来版本将支持从 analysis_result 中读取动态 BPM
        bpm_changes = [{"timestamp": 0.0, "bpm": bpm}]

        # ---- 组装谱面 ----
        chart = {
            "metadata": {
                "title": title,
                "artist": artist,
                "charter": "ORE Auto-Generator",  # 标记为自动生成
                "difficulty_name": difficulty_name,
                "difficulty_level": difficulty_level,
                "audio_file": audio_file,
                "analysis_file": "",
                "preview_start": 0.0,
            },
            "lane_count": lane_count,
            "bpm_changes": bpm_changes,
            "notes": notes,
        }

        self._chart = chart
        print(f"[ChartGenerator] 已生成谱面: {len(notes)} 个 note, {lane_count} 条轨道")
        return chart

    def save(self, filepath: str) -> str:
        """将生成的谱面保存为 ORE JSON 格式文件。

        自动创建目标目录（如果不存在）。
        保存前会清理内部字段（以 _ 开头的内部数据）。

        参数:
          filepath (str): 保存路径，如 "Charts/my_song.json"

        返回:
          str: 实际保存的绝对路径

        使用示例:
          path = generator.save("Charts/my_chart.json")
          print(f"谱面已保存到: {path}")
        """
        # 确保目标目录存在
        os.makedirs(os.path.dirname(filepath) or ".", exist_ok=True)

        # 清理内部字段（以 _ 开头的不写入 JSON）
        clean_chart = {k: v for k, v in self._chart.items()
                       if not k.startswith("_")}

        # 写入 JSON（UTF-8，2 空格缩进，保留中文）
        with open(filepath, "w", encoding="utf-8") as f:
            json.dump(clean_chart, f, indent=2, ensure_ascii=False)

        print(f"[ChartGenerator] 谱面已保存到: {filepath}")
        return filepath

    # ------------------------------------------------------------------
    # Internal Mapping Algorithms — 内部映射算法
    # ------------------------------------------------------------------

    def _map_onsets_to_notes(
        self,
        onset_times: np.ndarray,
        onset_strengths: np.ndarray,
        beat_times: np.ndarray,
        bpm: float,
        lane_count: int,
    ) -> List[Dict]:
        """将 onset 事件映射到游戏 note（核心算法）。

        映射策略:
          1. 过滤极弱 onset (strength < 0.15) — 避免谱面过于密集
          2. 轨道分配:
             - 强 onset (>0.7) → 中央轨道（更有强调感）
             - 弱 onset → 按节拍序号循环分配到各轨道
          3. Hold note 检测:
             - 如果相邻两个 onset 间隔在 (0.15s, 1.5*beat_dur] 内
               且前一个 onset 强度 > 0.5 → 标记为 Hold note
             - Hold 持续时长 = 间隔 * 0.7（保留一些间隔）

        参数:
          onset_times (np.ndarray): onset 时间数组
          onset_strengths (np.ndarray): onset 强度数组
          beat_times (np.ndarray): 节拍时间数组
          bpm (float): BPM 值
          lane_count (int): 轨道数量

        返回:
          List[Dict]: note 列表，每个 note 包含:
            {
              "timestamp": float,   # note 时间 (秒)
              "lane": int,          # 轨道编号 (0-based)
              "type": str,          # "Tap" 或 "Hold"
              "duration": float     # Hold 持续时长 (Tap=0.0)
            }
        """
        notes = []

        # 计算一拍时长 (秒)
        beat_dur = 60.0 / bpm

        for i, (t, strength) in enumerate(zip(onset_times, onset_strengths)):
            # ---- 过滤极弱 onset ----
            # 强度 < 0.15 的信号很可能是噪音或背景音
            if strength < 0.15:
                continue

            # ---- 轨道分配 ----
            # 找到最近的节拍，确定在 4/4 拍中的位置
            nearest_beat_idx = np.argmin(np.abs(beat_times - t))
            beat_in_measure = nearest_beat_idx % 4  # 该拍在 4/4 小节中的序号 (0-3)

            # 基础轨道 = 节拍序号 % 轨道数 (循环分配)
            base_lane = beat_in_measure % lane_count

            # ---- 强度调整: 强 onset → 中央轨道 ----
            if lane_count >= 4:
                center = lane_count // 2  # 中央轨道索引
                if strength > 0.7:
                    # 将强 onset 移到中央附近，根据奇偶微调
                    lane = center + (beat_in_measure % 2) * 2 - 1
                    lane = max(0, min(lane_count - 1, lane))
                else:
                    lane = base_lane
            else:
                # 轨道数小于 4 时直接用简单循环分配
                lane = base_lane % lane_count

            # ---- Note 类型检测 ----
            note_type = "Tap"
            duration = 0.0

            # Hold note 检测条件:
            #   1. 有下一个 onset
            #   2. 间隔在 0.15s ~ 1.5*beat_dur 之间
            #   3. 当前 onset 强度 > 0.5（强 onset 更适合做 Hold）
            if i + 1 < len(onset_times):
                gap = onset_times[i + 1] - t
                if 0.15 < gap <= beat_dur * 1.5 and strength > 0.5:
                    note_type = "Hold"
                    # Hold 持续时长为间隔的 70%（留一些间隙）
                    duration = min(gap * 0.7, beat_dur)

            notes.append({
                "timestamp": round(float(t), 3),
                "lane": int(lane),
                "type": note_type,
                "duration": round(duration, 3) if duration > 0 else 0.0,
            })

        # ---- 按时间排序 ----
        notes.sort(key=lambda n: n["timestamp"])

        # ---- 合并同时间 note 为和弦 ----
        # 当前版本预留，未来实现真正的和弦（多 note 同时间）
        notes = self._merge_chords(notes)

        return notes

    def _map_beats_to_notes(
        self, beat_times: np.ndarray, lane_count: int
    ) -> List[Dict]:
        """回退方案：将每个节拍位置映射为 Tap note。

        当 onset 检测效果不佳或音频缺乏显著 onset 时使用。
        将节拍按轨道循环分配，保证谱面节奏感。

        参数:
          beat_times (np.ndarray): 节拍时间数组
          lane_count (int): 轨道数量

        返回:
          List[Dict]: note 列表（全部为 Tap 类型）
        """
        notes = []
        for i, t in enumerate(beat_times):
            lane = i % lane_count  # 循环分配到各轨道
            notes.append({
                "timestamp": round(float(t), 3),
                "lane": int(lane),
                "type": "Tap",
                "duration": 0.0,
            })
        return notes

    def _merge_chords(self, notes: List[Dict]) -> List[Dict]:
        """合并同时间的 note 为和弦（当前为占位实现）。

        未来版本将实现真正的和弦功能：
          - 检测同时（或极近）的多个 note
          - 将这些 note 标记为同一和弦
          - 在谱面编辑器中显示为垂直排列的多个 note

        当前版本直接返回原列表。

        参数:
          notes (List[Dict]): note 列表

        返回:
          List[Dict]: 处理后的 note 列表（当前未做修改）
        """
        return notes

    # ------------------------------------------------------------------
    # Multi-difficulty Generation — 多难度生成
    # ------------------------------------------------------------------

    def generate_difficulty_set(
        self,
        analysis_result: Dict,
        title: str,
        artist: str,
        audio_file: str,
        lane_count: int,
    ) -> Dict[str, Dict]:
        """批量生成 Easy/Normal/Hard 三个难度谱面。

        各难度策略:
          - Easy (Lv1):   仅使用最强 50% onset (strength > 0.5)
          - Normal (Lv3): 使用全部 onset
          - Hard (Lv6):   在 Normal 基础上添加 16 分音符填充

        注意:
          难度设定是自适应的:
            - 如果原曲 onset 本来就少，Easy 和 Normal 可能很相似
            - Hard 添加的 16 分音符可能超出实际音乐内容

        参数:
          analysis_result (Dict): 分析结果字典
          title (str): 歌曲标题
          artist (str): 歌手/作者
          audio_file (str): 音频文件路径
          lane_count (int): 轨道数量

        返回:
          Dict[str, Dict]: {
            "Easy": chart_dict,
            "Normal": chart_dict,
            "Hard": chart_dict
          }

        使用示例:
          difficulties = generator.generate_difficulty_set(
              analysis, "Song", "Artist", "song.mp3", 4
          )
          for name, chart in difficulties.items():
              generator._chart = chart
              generator.save(f"Charts/song_{name.lower()}.json")
        """
        difficulties = {}

        # ---- Easy: 仅使用强 onset ----
        # 过滤条件: strength > 0.5
        easy_onsets = np.array(analysis_result.get("onset_times", []))
        easy_strengths = np.array(analysis_result.get("onset_strengths", []))
        mask = easy_strengths > 0.5  # 只保留较强的一半 onset
        easy_result = {
            **analysis_result,
            "onset_times": easy_onsets[mask].tolist(),
            "onset_strengths": easy_strengths[mask].tolist(),
        }
        difficulties["Easy"] = self.generate(
            easy_result, title, artist, audio_file, lane_count, "Easy", 1
        )

        # ---- Normal: 使用全部 onset ----
        difficulties["Normal"] = self.generate(
            analysis_result, title, artist, audio_file, lane_count, "Normal", 3
        )

        # ---- Hard: 在 onset 基础上添加 16 分音符 ----
        # 16 分音符 = 在每拍后 0.25 秒（约 16 分音符间隔）添加额外 note
        hard_result = {**analysis_result}
        if "onset_times" in analysis_result:
            extra_onsets = []
            for t in analysis_result["onset_times"]:
                extra_onsets.append(t + 0.25)  # 16 分音符间隔
            # 合并原始 onset 和新增 note
            hard_result["onset_times"] = sorted(
                list(analysis_result["onset_times"]) + extra_onsets
            )
        difficulties["Hard"] = self.generate(
            hard_result, title, artist, audio_file, lane_count, "Hard", 6
        )

        return difficulties

    @property
    def chart(self):
        """当前生成的谱面字典。

        Returns:
          Dict: ORE 标准格式的谱面数据
        """
        return self._chart