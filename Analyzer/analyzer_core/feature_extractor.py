"""Spectral Feature Extraction Module.

Extracts audio features: spectral centroid, bandwidth, RMS energy,
zero-crossing rate, MFCCs, chroma, etc.
Designed for future AI model training (Audio2Chart).
"""

import numpy as np
from typing import Dict


class FeatureExtractor:
    """Extracts spectral and temporal features from audio.

    Features are designed to be usable both for chart generation
    and as input features for future deep learning models.
    """

    def __init__(self):
        self._features = {}

    def extract(self, audio_data, sample_rate) -> Dict:
        """Extract all features and return as a dictionary."""
        import librosa

        n_fft = 2048
        hop_length = 512

        # Spectral features
        stft = np.abs(librosa.stft(audio_data, n_fft=n_fft, hop_length=hop_length))

        centroid = librosa.feature.spectral_centroid(
            S=stft, sr=sample_rate, n_fft=n_fft, hop_length=hop_length
        )
        bandwidth = librosa.feature.spectral_bandwidth(
            S=stft, sr=sample_rate, n_fft=n_fft, hop_length=hop_length
        )
        spectral_rolloff = librosa.feature.spectral_rolloff(
            S=stft, sr=sample_rate, n_fft=n_fft, hop_length=hop_length
        )

        # Energy / Loudness
        rms = librosa.feature.rms(S=stft)

        # Rhythm features
        zcr = librosa.feature.zero_crossing_rate(audio_data, hop_length=hop_length)

        # Harmonic/Percussive separation
        harmonic, percussive = librosa.effects.hpss(audio_data)

        # MFCCs (13 coefficients) - important for AI models
        mfcc = librosa.feature.mfcc(y=audio_data, sr=sample_rate, n_mfcc=13)

        # Chroma (12 pitch classes) - useful for chord/melody analysis
        chroma = librosa.feature.chroma_stft(
            S=stft, sr=sample_rate, hop_length=hop_length
        )

        # Aggregate features
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

        # Store per-frame data for AI training
        features["_frame_data"] = {
            "mfcc": mfcc.tolist(),
            "chroma": chroma.tolist(),
            "spectral_centroid": centroid[0].tolist(),
            "rms": rms[0].tolist(),
        }

        self._features = features
        print(f"[FeatureExtractor] Extracted {len(features) - 1} aggregate features")
        return features

    @property
    def features(self):
        return self._features