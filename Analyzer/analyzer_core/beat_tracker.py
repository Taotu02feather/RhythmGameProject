"""Beat and Downbeat Tracking Module.

Detects beat positions (quarter notes, eighth notes) and downbeats
(first beat of each measure) in audio signals.
"""

import numpy as np
from typing import Tuple


class BeatTracker:
    """Tracks beat positions from onset strength envelope.

    Identifies beat times, downbeat positions, and beat strengths
    for use in chart generation and game synchronization.
    """

    def __init__(self):
        self._beat_times = np.array([])
        self._downbeat_times = np.array([])
        self._beat_strengths = np.array([])

    def track(self, audio_data, sample_rate, bpm):
        """Track beat positions and return (beat_times, downbeat_times, beat_strengths)."""
        import librosa

        hop_length = 512
        onset_env = librosa.onset.onset_strength(
            y=audio_data, sr=sample_rate, hop_length=hop_length
        )
        tempo, beats = librosa.beat.beat_track(
            onset_envelope=onset_env, sr=sample_rate,
            hop_length=hop_length, start_bpm=bpm, tightness=100
        )
        beat_times = librosa.frames_to_time(beats, sr=sample_rate, hop_length=hop_length)

        strengths = []
        for beat_frame in beats:
            if beat_frame < len(onset_env):
                strengths.append(float(onset_env[beat_frame]))
            else:
                strengths.append(0.0)

        beat_strengths = np.array(strengths)
        if beat_strengths.max() > 0:
            beat_strengths /= beat_strengths.max()

        downbeat_indices = np.arange(0, len(beat_times), 4)
        downbeat_times = beat_times[downbeat_indices]

        self._beat_times = beat_times
        self._downbeat_times = downbeat_times
        self._beat_strengths = beat_strengths

        print(f"[BeatTracker] Found {len(beat_times)} beats, {len(downbeat_times)} downbeats")
        return beat_times, downbeat_times, beat_strengths

    @property
    def beat_times(self):
        return self._beat_times

    @property
    def downbeat_times(self):
        return self._downbeat_times

    @property
    def beat_strengths(self):
        return self._beat_strengths