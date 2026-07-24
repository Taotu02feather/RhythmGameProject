#!/usr/bin/env python3
"""
Open Rhythm Engine - Music Analyzer CLI

Usage:
    python run_analyzer.py <audio_file> [options]

Examples:
    python run_analyzer.py Songs/mysong.wav
    python run_analyzer.py Songs/mysong.mp3 --lanes 4 --title "My Song" --artist "Me"
    python run_analyzer.py Songs/mysong.wav --lanes 6 --difficulty-set
"""

import sys
import os
import json
import argparse

# Add Analyzer to path
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
    parser = argparse.ArgumentParser(
        description="Open Rhythm Engine - Music Analyzer",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Output files:
  <song>_analysis.json  - Full analysis results (BPM, beats, features, structure)
  <song>_chart.json      - Auto-generated rhythm game chart

The chart JSON can be loaded directly by the Open Rhythm Engine (C++ game).
        """
    )

    parser.add_argument("audio_file", help="Path to the audio file (WAV, MP3, FLAC, OGG)")
    parser.add_argument("--title", default=None, help="Song title (default: derived from filename)")
    parser.add_argument("--artist", default="Unknown Artist", help="Song artist")
    parser.add_argument("--lanes", type=int, default=4, choices=range(2, 9),
                        help="Number of lanes (2-8, default: 4)")
    parser.add_argument("--difficulty-name", default="Easy", help="Difficulty name")
    parser.add_argument("--difficulty-level", type=int, default=1, help="Difficulty level (1-10)")
    parser.add_argument("--difficulty-set", action="store_true",
                        help="Generate Easy/Normal/Hard difficulty set")
    parser.add_argument("--output-dir", default=None,
                        help="Output directory for generated files (default: Charts/)")
    parser.add_argument("--analysis-only", action="store_true",
                        help="Only generate analysis.json, skip chart generation")
    parser.add_argument("--sensitivity", type=float, default=0.5,
                        help="Onset detection sensitivity (0.0=more, 1.0=fewer, default: 0.5)")

    args = parser.parse_args()

    # Validate audio file
    if not os.path.exists(args.audio_file):
        print(f"ERROR: Audio file not found: {args.audio_file}")
        sys.exit(1)

    # Derive title from filename if not provided
    title = args.title or os.path.splitext(os.path.basename(args.audio_file))[0]

    # Determine output paths
    base_name = os.path.splitext(os.path.basename(args.audio_file))[0]
    output_dir = args.output_dir or "Charts"
    os.makedirs(output_dir, exist_ok=True)

    analysis_path = os.path.join(output_dir, f"{base_name}_analysis.json")
    chart_path = os.path.join(output_dir, f"{base_name}_chart.json")

    print("=" * 60)
    print("  Open Rhythm Engine - Music Analyzer v0.1.0")
    print("=" * 60)
    print(f"  Input:     {args.audio_file}")
    print(f"  Title:     {title}")
    print(f"  Artist:    {args.artist}")
    print(f"  Lanes:     {args.lanes}K")
    print(f"  Output:    {output_dir}/")
    print("=" * 60)

    # ======== Step 1: Load Audio ========
    print("\n[1/6] Loading audio...")
    loader = AudioLoader()
    try:
        audio, sr = loader.load(args.audio_file)
    except Exception as e:
        print(f"ERROR: Failed to load audio: {e}")
        sys.exit(1)

    # ======== Step 2: BPM Detection ========
    print("\n[2/6] Detecting BPM...")
    bpm_detector = BPMDetector()
    bpm, bpm_conf = bpm_detector.detect(audio, sr)

    # ======== Step 3: Beat Tracking ========
    print("\n[3/6] Tracking beats...")
    beat_tracker = BeatTracker()
    beat_times, downbeat_times, beat_strengths = beat_tracker.track(audio, sr, bpm)

    # ======== Step 4: Onset Detection ========
    print("\n[4/6] Detecting onsets...")
    onset_detector = OnsetDetector()
    onset_times, onset_strengths = onset_detector.detect(audio, sr, args.sensitivity)

    # ======== Step 5: Feature Extraction ========
    print("\n[5/6] Extracting audio features...")
    feature_extractor = FeatureExtractor()
    features = feature_extractor.extract(audio, sr)

    # ======== Step 6: Structure Analysis ========
    print("\n[6/6] Analyzing musical structure...")
    structure_analyzer = StructureAnalyzer()
    sections = structure_analyzer.analyze(audio, sr)

    # ======== Build Analysis Result ========
    analysis_result = {
        "source_file": args.audio_file,
        "analysis_version": "0.1.0",
        "duration_seconds": round(loader.duration, 3),
        "sample_rate": sr,
        "bpm": round(bpm, 1),
        "bpm_confidence": round(bpm_conf, 3),
        "beat_positions": [round(t, 3) for t in beat_times.tolist()],
        "downbeats": [round(t, 3) for t in downbeat_times.tolist()],
        "beat_strengths": [round(s, 3) for s in beat_strengths.tolist()],
        "onset_times": [round(t, 3) for t in onset_times.tolist()],
        "onset_strengths": [round(s, 3) for s in onset_strengths.tolist()],
        "spectral_features": {k: v for k, v in features.items() if not k.startswith("_")},
        "structure": {
            "sections": sections,
        },
    }

    # Save analysis JSON
    with open(analysis_path, "w", encoding="utf-8") as f:
        json.dump(analysis_result, f, indent=2, ensure_ascii=False)
    print(f"\n[ ✓ ] Analysis saved to: {analysis_path}")

    if args.analysis_only:
        print("\nAnalysis complete. Skipping chart generation (--analysis-only).")
        return 0

    # ======== Chart Generation ========
    print("\n========== Chart Generation ==========")

    chart_generator = ChartGenerator()

    # Relative path for audio file in chart
    rel_audio_file = os.path.relpath(args.audio_file, output_dir)

    if args.difficulty_set:
        # Generate Easy / Normal / Hard set
        difficulties = chart_generator.generate_difficulty_set(
            analysis_result, title, args.artist, rel_audio_file, args.lanes
        )
        for diff_name, chart_dict in difficulties.items():
            diff_path = os.path.join(
                output_dir, f"{base_name}_{diff_name.lower()}.json"
            )
            chart_generator._chart = chart_dict
            chart_generator.save(diff_path)

        # Update analysis with chart paths
        analysis_result["generated_charts"] = {
            name.lower(): f"{base_name}_{name.lower()}.json"
            for name in difficulties.keys()
        }
        with open(analysis_path, "w", encoding="utf-8") as f:
            json.dump(analysis_result, f, indent=2, ensure_ascii=False)

        print(f"\n[ ✓ ] Generated {len(difficulties)} difficulty levels!")
    else:
        # Generate single chart
        chart_dict = chart_generator.generate(
            analysis_result, title, args.artist, rel_audio_file,
            args.lanes, args.difficulty_name, args.difficulty_level
        )
        chart_generator.save(chart_path)

        # Update analysis with chart path
        analysis_result["generated_chart_path"] = os.path.basename(chart_path)
        with open(analysis_path, "w", encoding="utf-8") as f:
            json.dump(analysis_result, f, indent=2, ensure_ascii=False)

        print(f"\n[ ✓ ] Chart saved to: {chart_path}")

    print("\n" + "=" * 60)
    print("  Analysis complete! Next steps:")
    print(f"  1. Copy the chart file to your game's Charts/ directory")
    print(f"  2. Place your audio file in the game's Songs/ directory")
    print(f"  3. Launch Open Rhythm Engine to play!")
    print("=" * 60)

    return 0


if __name__ == "__main__":
    sys.exit(main())