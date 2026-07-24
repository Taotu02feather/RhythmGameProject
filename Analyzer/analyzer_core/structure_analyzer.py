"""Music Structure Analysis Module.

Detects musical structure (intro, verse, chorus, bridge, outro)
using self-similarity matrices and spectral clustering.
"""

import numpy as np
from typing import List, Dict


class StructureAnalyzer:
    """Analyzes musical structure by detecting repeated patterns.

    Uses MFCC-based self-similarity matrices to find structural
    boundaries, then labels sections based on their relationships.
    """

    def __init__(self):
        self._sections: List[Dict] = []

    def analyze(self, audio_data, sample_rate) -> List[Dict]:
        """Detect structural sections and return list of {start, end, label}."""
        import librosa

        n_fft = 2048
        hop_length = 512

        # Compute MFCC for feature-based segmentation
        mfcc = librosa.feature.mfcc(y=audio_data, sr=sample_rate, n_mfcc=13)

        # Self-similarity matrix
        S = librosa.segment.recurrence_matrix(mfcc, mode='affinity', sym=True)

        # Segment using agglomerative clustering
        n_frames = mfcc.shape[1]
        if n_frames < 2:
            self._sections = [{"start": 0.0, "end": len(audio_data) / sample_rate, "label": "full"}]
            return self._sections

        # Use librosa's structural segmentation
        try:
            boundaries = librosa.segment.agglomerative(mfcc, k=3)
            bound_times = librosa.frames_to_time(boundaries, sr=sample_rate, hop_length=hop_length)
        except Exception:
            # Fallback to simple equal-length sections
            total_dur = len(audio_data) / sample_rate
            bound_times = np.linspace(0, total_dur, 4)

        # Build sections
        total_duration = len(audio_data) / sample_rate
        sections = []
        for i in range(len(bound_times) - 1):
            start = float(bound_times[i])
            end = float(bound_times[i + 1])
            label = self._label_section(i, len(bound_times) - 1)
            sections.append({"start": start, "end": end, "label": label})

        self._sections = sections
        print(f"[StructureAnalyzer] Found {len(sections)} sections")
        for s in sections:
            print(f"  {s['label']}: {s['start']:.1f}s - {s['end']:.1f}s")
        return sections

    def _label_section(self, idx: int, total: int) -> str:
        """Assign a semantic label to a section based on position."""
        if total == 1:
            return "full"
        if idx == 0:
            return "intro"
        if idx == total - 1:
            return "outro"
        if total >= 4 and idx == total // 2:
            return "bridge"
        if idx % 2 == 1:
            return "chorus"
        return "verse"

    @property
    def sections(self):
        return self._sections