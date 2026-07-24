"""BPM (Beats Per Minute / Tempo) Detection Module.

Uses onset-based and autocorrelation methods via librosa
to estimate the global and local tempo of a music track.
"""

import numpy as np
from typing import Tuple, Optional


class BPMDetector:
    """Estimates the primary tempo (BPM) of an audio track.

    Combines onset-based tempo estimation with autocorrelation
    for robust BPM detection across various genres.
    """

    def __init__(self):
        self._bpm: float = 120.0
        self._confidence: float = 0.0
        self._dynamic_tempo: bool = False

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def detect(self, audio_data: np.ndarray, sample_rate: int) -> Tuple[float, float]:
        """Detect the main BPM and return (bpm, confidence).

        Args:
            audio_data: Mono audio signal (float32, normalized).
            sample_rate: Sample rate in Hz.

        Returns:
            (bpm, confidence) where confidence is 0.0 to 1.0.
        """
        import librosa

        # Method 1: Onset-based tempo estimation
        onset_env = librosa.onset.onset_strength(
            y=audio_data, sr=sample_rate, hop_length=512
        )

        # Estimate tempo from onset strength
        tempo, beats = librosa.beat.beat_track(
            onset_envelope=onset_env,
            sr=sample_rate,
            hop_length=512,
            tightness=100  # Higher = stricter tempo
        )

        # librosa returns scalar or array
        if isinstance(tempo, np.ndarray):
            bpm = float(tempo[0])
        else:
            bpm = float(tempo)

        # Clamp BPM to reasonable range
        bpm = max(40.0, min(300.0, bpm))

        # Confidence heuristic based on beat consistency
        confidence = self._compute_confidence(beats, onset_env, bpm, sample_rate)

        self._bpm = bpm
        self._confidence = confidence

        print(f"[BPMDetector] Estimated BPM: {bpm:.1f} (confidence: {confidence:.2f})")
        return bpm, confidence

    def detect_dynamic(
        self, audio_data: np.ndarray, sample_rate: int, window_sec: float = 4.0
    ) -> np.ndarray:
        """Detect dynamic BPM over time (for songs with tempo changes).

        Returns an array of (time, bpm) pairs.
        """
        import librosa

        hop_length = 512
        frame_length = int(window_sec * sample_rate / hop_length)

        onset_env = librosa.onset.onset_strength(
            y=audio_data, sr=sample_rate, hop_length=hop_length
        )

        # Sliding window tempo estimation
        tempos = []
        for i in range(0, len(onset_env), frame_length // 2):
            segment = onset_env[i : i + frame_length]
            if len(segment) < frame_length // 2:
                break
            tempo = librosa.beat.tempo(
                onset_envelope=segment,
                sr=sample_rate,
                hop_length=hop_length
            )
            t = librosa.frames_to_time(i, sr=sample_rate, hop_length=hop_length)
            tempos.append((t, float(tempo[0] if isinstance(tempo, np.ndarray) else tempo)))

        self._dynamic_tempo = True
        return np.array(tempos)

    # ------------------------------------------------------------------
    # Properties
    # ------------------------------------------------------------------

    @property
    def bpm(self) -> float:
        """The detected global BPM."""
        return self._bpm

    @property
    def confidence(self) -> float:
        """Confidence of the BPM estimation (0.0 - 1.0)."""
        return self._confidence

    # ------------------------------------------------------------------
    # Internal Helpers
    # ------------------------------------------------------------------

    def _compute_confidence(
        self,
        beats: np.ndarray,
        onset_env: np.ndarray,
        bpm: float,
        sample_rate: int
    ) -> float:
        """Compute a heuristic confidence score for the BPM estimate."""
        if len(beats) < 2:
            return 0.0

        import librosa

        beat_times = librosa.frames_to_time(beats, sr=sample_rate, hop_length=512)

        if len(beat_times) < 2:
            return 0.0

        # Compute inter-beat intervals
        ibis = np.diff(beat_times)
        ideal_ibi = 60.0 / bpm

        # How close are IBIs to the ideal value?
        deviations = np.abs(ibis - ideal_ibi) / ideal_ibi
        # Acceptable deviation: within 20% of ideal
        good_beats = np.sum(deviations < 0.2)
        ratio = good_beats / len(ibis)

        # Scale: higher ratio = higher confidence
        return min(1.0, ratio * 1.2)