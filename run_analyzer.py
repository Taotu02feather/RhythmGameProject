#!/usr/bin/env python3
"""
Open Rhythm Engine — 音乐分析器命令行入口 (run_analyzer.py)
=============================================================

本脚本是 Open Rhythm Engine Python 分析系统的命令行入口。

功能概述:
  用户运行此脚本并传入一个音乐文件路径后，系统自动执行完整的
  6 步分析流程，最终生成可被 C++ 引擎加载的游戏谱面。

分析流程（6 步）:
  第1步 - AudioLoader.load():
           读取 WAV/MP3/FLAC/OGG 文件，转换为单声道 float32 数组
  第2步 - BPMDetector.detect():
           使用 librosa 的 onset 强度 + 自相关算法估计歌曲 BPM
  第3步 - BeatTracker.track():
           在已知 BPM 指导下精确定位每个节拍的时间位置
  第4步 - OnsetDetector.detect():
           使用频谱通量法检测音符起始点（鼓点/敲击）
  第5步 - FeatureExtractor.extract():
           提取 MFCC(13维)、Chroma、频谱质心、RMS 能量等特征
  第6步 - StructureAnalyzer.analyze():
           使用自相似矩阵 + 凝聚聚类分割 intro/verse/chorus 等段落

输出文件:
  Charts/<歌名>_analysis.json  — 完整音乐分析数据 (BPM/节拍/onset/特征/结构)
  Charts/<歌名>_chart.json      — 自动生成的可玩游戏谱面 (ORE 标准格式)

使用示例:
  # 基础用法 — 分析并生成 4K 谱面
  python run_analyzer.py Songs/mysong.mp3

  # 生成 6K 多难度谱面
  python run_analyzer.py Songs/mysong.mp3 --lanes 6 --difficulty-set

  # 指定歌曲元数据
  python run_analyzer.py Songs/mysong.mp3 --title "我的歌" --artist "乐队名"

  # 仅生成分析数据（不生成谱面）
  python run_analyzer.py Songs/mysong.mp3 --analysis-only

  # 所有參數
  python run_analyzer.py --help
"""

import sys
import os
import json
import argparse

# 将 Analyzer 包加入 Python 搜索路径
# 确保可以直接 import Analyzer.analyzer_core 中的模块
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from Analyzer.analyzer_core import (
    AudioLoader,
    BPMDetector,
    BeatTracker,
    OnsetDetector,
    FeatureExtractor,
    StructureAnalyzer,
    ChartGenerator,
)


def main():
    """分析器主函数 — 解析命令行参数，执行 6 步分析并生成谱面。

    返回值:
      int: 0 = 成功, 1 = 失败

    流程概述:
      1. 解析命令行参数 (argparse)
      2. 验证输入文件存在
      3. 依次执行 6 个分析步骤
      4. 保存 analysis.json
      5. 生成 chart.json（除非指定 --analysis-only）
    """
    # ---- 解析命令行参数 ----
    parser = argparse.ArgumentParser(
        description="Open Rhythm Engine - 音乐分析器",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
输出文件:
  <歌名>_analysis.json  — 完整分析结果 (BPM, 节拍, onset, 特征, 结构)
  <歌名>_chart.json      — 自动生成的节奏游戏谱面

生成的谱面可直接被 Open Rhythm Engine (C++ 游戏程序) 加载游玩。
        """
    )

    # 必需参数
    parser.add_argument(
        "audio_file",
        help="音频文件路径 (支持 WAV, MP3, FLAC, OGG)"
    )

    # 可选参数
    parser.add_argument(
        "--title", default=None,
        help="歌曲标题 (默认: 从文件名推断)"
    )
    parser.add_argument(
        "--artist", default="Unknown Artist",
        help="歌曲作者/歌手名"
    )
    parser.add_argument(
        "--lanes", type=int, default=4, choices=range(2, 9),
        help="轨道数量 2-8 (默认: 4 = 4K 模式)"
    )
    parser.add_argument(
        "--difficulty-name", default="Easy",
        help="难度名称 (默认: Easy)"
    )
    parser.add_argument(
        "--difficulty-level", type=int, default=1,
        help="难度数字等级 1-10 (默认: 1)"
    )
    parser.add_argument(
        "--difficulty-set", action="store_true",
        help="生成 Easy/Normal/Hard 三难度套装"
    )
    parser.add_argument(
        "--output-dir", default=None,
        help="输出目录 (默认: Charts/)"
    )
    parser.add_argument(
        "--analysis-only", action="store_true",
        help="仅生成 analysis.json，跳过谱面生成"
    )
    parser.add_argument(
        "--sensitivity", type=float, default=0.5,
        help="Onset 检测敏感度 0.0-1.0 (默认: 0.5，越低检测越多 onset)"
    )

    args = parser.parse_args()

    # ---- 验证输入文件 ----
    if not os.path.exists(args.audio_file):
        print(f"错误: 找不到音频文件: {args.audio_file}")
        sys.exit(1)

    # ---- 确定歌曲标题 ----
    # 如果未指定标题，从文件名推断（去掉扩展名）
    title = args.title or os.path.splitext(os.path.basename(args.audio_file))[0]

    # ---- 确定输出路径 ----
    # 基础文件名 = 歌曲文件名（不含扩展名）
    base_name = os.path.splitext(os.path.basename(args.audio_file))[0]
    output_dir = args.output_dir or "Charts"
    os.makedirs(output_dir, exist_ok=True)

    analysis_path = os.path.join(output_dir, f"{base_name}_analysis.json")
    chart_path = os.path.join(output_dir, f"{base_name}_chart.json")

    # ---- 输出分析参数 ----
    print("=" * 60)
    print("  Open Rhythm Engine - 音乐分析器 v0.1.0")
    print("=" * 60)
    print(f"  输入文件:  {args.audio_file}")
    print(f"  歌曲标题:  {title}")
    print(f"  歌手:      {args.artist}")
    print(f"  轨道数:    {args.lanes}K")
    print(f"  输出目录:  {output_dir}/")
    print(f"  敏感度:    {args.sensitivity}")
    if args.difficulty_set:
        print(f"  难度:      Easy/Normal/Hard 套件")
    else:
        print(f"  难度:      {args.difficulty_name} (Lv.{args.difficulty_level})")
    print("=" * 60)

    # ================================================================
    # 第1步: 加载音频
    # ================================================================
    print("\n[1/6] 正在加载音频文件...")
    loader = AudioLoader()
    try:
        audio, sr = loader.load(args.audio_file)
    except Exception as e:
        print(f"错误: 音频加载失败: {e}")
        sys.exit(1)

    # ================================================================
    # 第2步: BPM 检测
    # ================================================================
    print("\n[2/6] 正在检测 BPM（歌曲速度）...")
    bpm_detector = BPMDetector()
    bpm, bpm_conf = bpm_detector.detect(audio, sr)

    # ================================================================
    # 第3步: 节拍跟踪
    # ================================================================
    print("\n[3/6] 正在跟踪节拍位置...")
    beat_tracker = BeatTracker()
    beat_times, downbeat_times, beat_strengths = beat_tracker.track(audio, sr, bpm)

    # ================================================================
    # 第4步: Onset (音符起始点) 检测
    # ================================================================
    print("\n[4/6] 正在检测音符起始点（鼓点）...")
    onset_detector = OnsetDetector()
    onset_times, onset_strengths = onset_detector.detect(
        audio, sr, args.sensitivity
    )

    # ================================================================
    # 第5步: 频谱特征提取
    # ================================================================
    print("\n[5/6] 正在提取音频频谱特征...")
    feature_extractor = FeatureExtractor()
    features = feature_extractor.extract(audio, sr)

    # ================================================================
    # 第6步: 音乐结构分析
    # ================================================================
    print("\n[6/6] 正在分析音乐结构...")
    structure_analyzer = StructureAnalyzer()
    sections = structure_analyzer.analyze(audio, sr)

    # ================================================================
    # 构建分析结果
    # ================================================================
    analysis_result = {
        # 元信息
        "source_file": args.audio_file,
        "analysis_version": "0.1.0",
        "duration_seconds": round(loader.duration, 3),
        "sample_rate": sr,

        # BPM 检测结果
        "bpm": round(bpm, 1),
        "bpm_confidence": round(bpm_conf, 3),

        # 节拍数据
        "beat_positions": [round(t, 3) for t in beat_times.tolist()],
        "downbeats": [round(t, 3) for t in downbeat_times.tolist()],
        "beat_strengths": [round(s, 3) for s in beat_strengths.tolist()],

        # Onset 数据
        "onset_times": [round(t, 3) for t in onset_times.tolist()],
        "onset_strengths": [round(s, 3) for s in onset_strengths.tolist()],

        # 频谱特征（排除内部帧数据 _frame_data）
        "spectral_features": {
            k: v for k, v in features.items() if not k.startswith("_")
        },

        # 音乐结构
        "structure": {
            "sections": sections,
        },
    }

    # ---- 保存 analysis.json ----
    with open(analysis_path, "w", encoding="utf-8") as f:
        json.dump(analysis_result, f, indent=2, ensure_ascii=False)
    print(f"\n[ ✓ ] 分析数据已保存到: {analysis_path}")

    # ---- 如果指定 --analysis-only，跳过谱面生成 ----
    if args.analysis_only:
        print("\n分析完成。已跳过谱面生成 (--analysis-only 已指定)。")
        return 0

    # ================================================================
    # 谱面生成
    # ================================================================
    print("\n========== 谱面生成 ==========")

    chart_generator = ChartGenerator()

    # 音频文件在谱面中的相对路径（相对于谱面文件所在目录）
    rel_audio_file = os.path.relpath(args.audio_file, output_dir)

    if args.difficulty_set:
        # ---- 多难度生成: Easy / Normal / Hard ----
        difficulties = chart_generator.generate_difficulty_set(
            analysis_result, title, args.artist, rel_audio_file, args.lanes
        )

        # 保存每个难度谱面到独立文件
        for diff_name, chart_dict in difficulties.items():
            diff_path = os.path.join(
                output_dir, f"{base_name}_{diff_name.lower()}.json"
            )
            chart_generator._chart = chart_dict
            chart_generator.save(diff_path)

        # 更新分析结果，记录生成的谱面路径
        analysis_result["generated_charts"] = {
            name.lower(): f"{base_name}_{name.lower()}.json"
            for name in difficulties.keys()
        }
        with open(analysis_path, "w", encoding="utf-8") as f:
            json.dump(analysis_result, f, indent=2, ensure_ascii=False)

        print(f"\n[ ✓ ] 已生成 {len(difficulties)} 个难度级别的谱面!")
    else:
        # ---- 单难度谱面生成 ----
        chart_dict = chart_generator.generate(
            analysis_result, title, args.artist, rel_audio_file,
            args.lanes, args.difficulty_name, args.difficulty_level
        )
        chart_generator.save(chart_path)

        # 更新分析结果
        analysis_result["generated_chart_path"] = os.path.basename(chart_path)
        with open(analysis_path, "w", encoding="utf-8") as f:
            json.dump(analysis_result, f, indent=2, ensure_ascii=False)

        print(f"\n[ ✓ ] 谱面已保存到: {chart_path}")

    # ---- 完成提示 ----
    print("\n" + "=" * 60)
    print("  分析完成！下一步操作:")
    print(f"  1. 将谱面文件复制到游戏引擎的 Charts/ 目录")
    print(f"  2. 将音频文件放入游戏引擎的 Songs/ 目录")
    print(f"  3. 启动 Open Rhythm Engine 开始游戏!")
    print("=" * 60)

    return 0


if __name__ == "__main__":
    # 脚本入口: 调用 main() 并用 sys.exit() 传递返回码
    sys.exit(main())