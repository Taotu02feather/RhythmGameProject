#pragma once

#include <SDL_mixer.h>
#include <string>
#include <memory>

namespace Ore {

struct AudioConfig {
    int sampleRate = 44100;
    int channels = 2;
    int chunkSize = 2048;
};

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    bool Initialize(const AudioConfig& config = AudioConfig{});
    void Shutdown();

    // Music playback (long audio, one at a time)
    Mix_Music* LoadMusic(const std::string& filepath);
    void PlayMusic(Mix_Music* music, int loops = 0);
    void StopMusic();
    void PauseMusic();
    void ResumeMusic();
    bool IsMusicPlaying() const;

    // Sound effects (short audio, multiple at once)
    Mix_Chunk* LoadSound(const std::string& filepath);
    int PlaySound(Mix_Chunk* sound, int loops = 0);

    void SetMusicVolume(int volume);  // 0 - 128
    void SetSoundVolume(int volume);  // 0 - 128

    // Free resources
    void FreeMusic(Mix_Music* music);
    void FreeSound(Mix_Chunk* sound);

private:
    bool m_initialized = false;
};

} // namespace Ore