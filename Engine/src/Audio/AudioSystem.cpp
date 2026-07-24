#include "AudioSystem.h"
#include <iostream>

namespace Ore {

AudioSystem::AudioSystem() = default;

AudioSystem::~AudioSystem() {
    Shutdown();
}

bool AudioSystem::Initialize(const AudioConfig& config) {
    int flags = MIX_INIT_MP3 | MIX_INIT_OGG;
    int initted = Mix_Init(flags);
    if ((initted & flags) != flags) {
        std::cerr << "Mix_Init failed: " << Mix_GetError() << std::endl;
        std::cerr << "Audio init flags missing. MP3/OGG support may be limited." << std::endl;
    }

    if (Mix_OpenAudio(config.sampleRate, MIX_DEFAULT_FORMAT, config.channels, config.chunkSize) < 0) {
        std::cerr << "Mix_OpenAudio failed: " << Mix_GetError() << std::endl;
        return false;
    }

    Mix_AllocateChannels(16);
    m_initialized = true;

    std::cout << "AudioSystem initialized ("
              << config.sampleRate << " Hz, "
              << config.channels << " channels)." << std::endl;
    return true;
}

void AudioSystem::Shutdown() {
    if (m_initialized) {
        Mix_CloseAudio();
        Mix_Quit();
        m_initialized = false;
    }
}

Mix_Music* AudioSystem::LoadMusic(const std::string& filepath) {
    if (!m_initialized) return nullptr;
    Mix_Music* music = Mix_LoadMUS(filepath.c_str());
    if (!music) {
        std::cerr << "Failed to load music: " << filepath << " - " << Mix_GetError() << std::endl;
    }
    return music;
}

void AudioSystem::PlayMusic(Mix_Music* music, int loops) {
    if (!m_initialized || !music) return;
    if (Mix_PlayMusic(music, loops) < 0) {
        std::cerr << "Mix_PlayMusic failed: " << Mix_GetError() << std::endl;
    }
}

void AudioSystem::StopMusic() {
    Mix_HaltMusic();
}

void AudioSystem::PauseMusic() {
    Mix_PauseMusic();
}

void AudioSystem::ResumeMusic() {
    Mix_ResumeMusic();
}

bool AudioSystem::IsMusicPlaying() const {
    return Mix_PlayingMusic() != 0;
}

Mix_Chunk* AudioSystem::LoadSound(const std::string& filepath) {
    if (!m_initialized) return nullptr;
    Mix_Chunk* sound = Mix_LoadWAV(filepath.c_str());
    if (!sound) {
        std::cerr << "Failed to load sound: " << filepath << " - " << Mix_GetError() << std::endl;
    }
    return sound;
}

int AudioSystem::PlaySound(Mix_Chunk* sound, int loops) {
    if (!m_initialized || !sound) return -1;
    int channel = Mix_PlayChannel(-1, sound, loops);
    if (channel < 0) {
        std::cerr << "Mix_PlayChannel failed: " << Mix_GetError() << std::endl;
    }
    return channel;
}

void AudioSystem::SetMusicVolume(int volume) {
    Mix_VolumeMusic(volume);
}

void AudioSystem::SetSoundVolume(int volume) {
    Mix_Volume(-1, volume);
}

void AudioSystem::FreeMusic(Mix_Music* music) {
    if (music) Mix_FreeMusic(music);
}

void AudioSystem::FreeSound(Mix_Chunk* sound) {
    if (sound) Mix_FreeChunk(sound);
}

} // namespace Ore