"""Automatic Chart Generation Module.

Generates rhythm game charts from analysis results.
Maps musical events (onsets, beats) to game notes across lanes.
Supports configurable lane counts (2K-8K).
"""

import json
import os
import numpy as np
from typing import Dict, List


class ChartGenerator:
    """Generates rhythm game chart JSON from music analysis results.

    Takes BPM, beats, onsets, and structure analysis results and
    produces a playable chart file in the ORE chart format.

    Future: This generator will serve as a baseline for training
    Audio2Chart deep learning models.
    """

    def __init__(self):
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
        """Generate a complete chart dictionary from analysis data.

        Args:
            analysis_result: Dict containing bpm, beat_positions, onset_times, etc.
            title: Song title.
            artist: Song artist.
            audio_file: Relative path to the audio file.
            lane_count: Number of lanes (2-8).
            difficulty_name: e.g. "Easy", "Normal", "Hard".
            difficulty_level: Numeric difficulty rating.

        Returns:
            Complete chart dictionary ready to be saved as JSON.
        """
        # Extract analysis data
        bpm = analysis_result.get("bpm", 120.0)
        beat_times = np.array(analysis_result.get("beat_positions", []))
        onset_times = np.array(analysis_result.get("onset_times", []))
        onset_strengths = np.array(analysis_result.get("onset_strengths", []))

        notes = []

        if len(onset_times) > 0 and len(beat_times) > 0:
            notes = self._map_onsets_to_notes(
                onset_times, onset_strengths, beat_times, bpm, lane_count
            )
        elif len(beat_times) > 0:
            # Fallback: use beats directly
            notes = self._map_beats_to_notes(beat_times, lane_count)

        # Build BPM changes
        bpm_changes = [{"timestamp": 0.0, "bpm": bpm}]

        # Build chart
        chart = {
            "metadata": {
                "title": title,
                "artist": artist,
                "charter": "ORE Auto-Generator",
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
        print(f"[ChartGenerator] Generated chart: {len(notes)} notes, {lane_count} lanes")
        return chart

    def save(self, filepath: str) -> str:
        """Save the generated chart to a JSON file."""
        os.makedirs(os.path.dirname(filepath) or ".", exist_ok=True)

        # Remove internal frame data before saving
        clean_chart = {k: v for k, v in self._chart.items()
                       if not k.startswith("_")}

        with open(filepath, "w", encoding="utf-8") as f:
            json.dump(clean_chart, f, indent=2, ensure_ascii=False)

        print(f"[ChartGenerator] Chart saved to: {filepath}")
        return filepath

    # ------------------------------------------------------------------
    # Internal Mapping Algorithms
    # ------------------------------------------------------------------

    def _map_onsets_to_notes(
        self,
        onset_times: np.ndarray,
        onset_strengths: np.ndarray,
        beat_times: np.ndarray,
        bpm: float,
        lane_count: int,
    ) -> List[Dict]:
        """Map onset events to game notes across lanes.

        Strategy:
        - High-strength onsets on downbeats -> center lanes
        - Lower-strength onsets -> outer lanes
        - Closely-spaced onsets -> chords (simultaneous notes)
        """
        notes = []

        # Beat duration in seconds
        beat_dur = 60.0 / bpm

        for i, (t, strength) in enumerate(zip(onset_times, onset_strengths)):
            if strength < 0.15:
                continue  # Skip very weak onsets

            # Determine lane based on:
            # 1) Position within measure (mod 4 beat)
            # 2) Strength of onset
            nearest_beat_idx = np.argmin(np.abs(beat_times - t))
            beat_in_measure = nearest_beat_idx % 4

            # Base lane assignment
            base_lane = beat_in_measure % lane_count

            # Adjust by strength: stronger hits -> center lanes
            if lane_count >= 4:
                center = lane_count // 2
                if strength > 0.7:
                    lane = center + (beat_in_measure % 2) * 2 - 1
                    lane = max(0, min(lane_count - 1, lane))
                else:
                    lane = base_lane
            else:
                lane = base_lane % lane_count

            # Determine note type
            note_type = "Tap"
            duration = 0.0

            # Check if this onset is close to the next one (potential hold note)
            if i + 1 < len(onset_times):
                gap = onset_times[i + 1] - t
                if 0.15 < gap <= beat_dur * 1.5 and strength > 0.5:
                    note_type = "Hold"
                    duration = min(gap * 0.7, beat_dur)

            notes.append({
                "timestamp": round(float(t), 3),
                "lane": int(lane),
                "type": note_type,
                "duration": round(duration, 3) if duration > 0 else 0.0,
            })

        # Sort by timestamp
        notes.sort(key=lambda n: n["timestamp"])

        # Merge simultaneous notes into chords (same timestamp)
        notes = self._merge_chords(notes)

        return notes

    def _map_beats_to_notes(
        self, beat_times: np.ndarray, lane_count: int
    ) -> List[Dict]:
        """Fallback: Place Tap notes on each beat.

        Simple pattern for songs with minimal onset detection.
        """
        notes = []
        for i, t in enumerate(beat_times):
            lane = i % lane_count
            notes.append({
                "timestamp": round(float(t), 3),
                "lane": int(lane),
                "type": "Tap",
                "duration": 0.0,
            })
        return notes

    def _merge_chords(self, notes: List[Dict]) -> List[Dict]:
        """Simple pass-through for now. Chord merging deferred to editor."""
        return notes

    # ------------------------------------------------------------------
    # Multi-difficulty Generation (future use)
    # ------------------------------------------------------------------

    def generate_difficulty_set(
        self,
        analysis_result: Dict,
        title: str,
        artist: str,
        audio_file: str,
        lane_count: int,
    ) -> Dict[str, Dict]:
        """Generate a complete difficulty set (Easy, Normal, Hard).

        Returns dict of difficulty_name -> chart_dict.
        """
        difficulties = {}

        # Easy: fewer notes, only strong onsets
        easy_onsets = np.array(analysis_result.get("onset_times", []))
        easy_strengths = np.array(analysis_result.get("onset_strengths", []))
        mask = easy_strengths > 0.5
        easy_result = {
            **analysis_result,
            "onset_times": easy_onsets[mask].tolist(),
            "onset_strengths": easy_strengths[mask].tolist(),
        }
        difficulties["Easy"] = self.generate(
            easy_result, title, artist, audio_file, lane_count, "Easy", 1
        )

        # Normal: all detected onsets
        difficulties["Normal"] = self.generate(
            analysis_result, title, artist, audio_file, lane_count, "Normal", 3
        )

        # Hard: add notes between beats for density
        hard_result = {**analysis_result}
        if "onset_times" in analysis_result:
            extra_onsets = []
            for t in analysis_result["onset_times"]:
                extra_onsets.append(t + 0.25)  # Add 16th notes
            hard_result["onset_times"] = sorted(
                list(analysis_result["onset_times"]) + extra_onsets
            )
        difficulties["Hard"] = self.generate(
            hard_result, title, artist, audio_file, lane_count, "Hard", 6
        )

        return difficulties

    @property
    def chart(self):
        return self._chart