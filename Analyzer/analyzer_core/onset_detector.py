"""Onset / Drum Hit Detection Module.

Detects musical onsets (note attacks, drum hits, percussive events)
in audio signals using spectral flux and energy-based methods.
"""

import numpy as np
from typing import Tuple


class OnsetDetector:
    """Detects onset events (drum hits, note attacks) in audio.

    Uses librosa's onset detection with spectral flux method.
    Provides onset times and their strengths for chart generation.
    """

    def __init__(self):
        self._onset_times = np.array([])
        self._onset_strengths = np.array([])

    def detect(self, audio_data, sample_rate, sensitivity=0.5):
        """Detect onset events and return (onset_times, onset_strengths)."""
        import librosa

        onset_env = librosa.onset.onset_strength(
            y=audio_data, sr=sample_rate, hop_length=512,
            aggregate=np.median
        )

        onset_frames = librosa.onset.onset_detect(
            onset_envelope=onset_env, sr=sample_rate,
            hop_length=512, delta=sensitivity * 0.3, backtrack=True
        )

        onset_times = librosa.frames_to_time(onset_frames, sr=sample_rate, hop_length=512)

        strengths = []
        for frame in onset_frames:
            if frame < len(onset_env):
                strengths.append(float(onset_env[frame]))
            else:
                strengths.append(0.0)

        onset_strengths = np.array(strengths)
        if len(onset_strengths) > 0 and onset_strengths.max() > 0:
            onset_strengths /= onset_strengths.max()

        self._onset_times = onset_times
        self._onset_strengths = onset_strengths

        print(f"[OnsetDetector] Detected {len(onset_times)} onsets")
        return onset_times, onset_strengths

    def detect_per_band(self, audio_data, sample_rate, num_bands=3):
        """Detect onsets in separate frequency bands."""
        import librosa

        onset_envs = librosa.onset.onset_strength_multi(
            y=audio_data, sr=sample_rate, hop_length=512, channels=num_bands
        )

        results = []
        for band_idx in range(num_bands):
            onset_frames = librosa.onset.onset_detect(
                onset_envelope=onset_envs[band_idx], sr=sample_rate,
                hop_length=512, delta=0.15, backtrack=True
            )
            onset_times = librosa.frames_to_time(onset_frames, sr=sample_rate, hop_length=512)

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

    @property
    def onset_times(self):
        return self._onset_times

    @property
    def onset_strengths(self):
        return self._onset_strengths