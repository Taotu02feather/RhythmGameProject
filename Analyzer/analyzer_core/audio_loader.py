"""Audio file loading and preprocessing module.

Supports WAV, MP3, FLAC, OGG formats via librosa.
Provides unified interface for audio data access across all analysis modules.
"""

import os
import numpy as np
from typing import Tuple, Optional


class AudioLoader:
    """Loads and preprocesses audio files for analysis.

    Wraps librosa for robust multi-format audio loading.
    Provides caching of loaded data for efficient multi-pass analysis.
    """

    def __init__(self):
        self._sample_rate: int = 44100
        self._audio_data: Optional[np.ndarray] = None
        self._duration: float = 0.0
        self._filepath: str = ""

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def load(self, filepath: str, target_sr: int = 44100) -> Tuple[np.ndarray, int]:
        """Load an audio file and return (audio_data, sample_rate).

        Audio data is returned as a mono float32 numpy array normalized to [-1, 1].
        """
        import librosa

        if not os.path.exists(filepath):
            raise FileNotFoundError(f"Audio file not found: {filepath}")

        self._filepath = filepath

        # Load with librosa (handles WAV, MP3, FLAC, OGG, etc.)
        audio, sr = librosa.load(filepath, sr=target_sr, mono=True)

        self._audio_data = audio.astype(np.float32)
        self._sample_rate = sr
        self._duration = len(audio) / sr

        print(f"[AudioLoader] Loaded: {filepath}")
        print(f"  Sample rate: {sr} Hz")
        print(f"  Duration: {self._duration:.2f}s")
        print(f"  Samples: {len(audio)}")

        return self._audio_data, self._sample_rate

    @property
    def audio_data(self) -> Optional[np.ndarray]:
        """The raw mono audio signal (float32, normalized)."""
        return self._audio_data

    @property
    def sample_rate(self) -> int:
        """Sample rate of the loaded audio."""
        return self._sample_rate

    @property
    def duration(self) -> float:
        """Duration of the audio in seconds."""
        return self._duration

    @property
    def filepath(self) -> str:
        """Original file path of the loaded audio."""
        return self._filepath

    def time_to_sample(self, time_sec: float) -> int:
        """Convert a timestamp in seconds to a sample index."""
        return int(time_sec * self._sample_rate)

    def sample_to_time(self, sample_idx: int) -> float:
        """Convert a sample index to a timestamp in seconds."""
        return sample_idx / self._sample_rate