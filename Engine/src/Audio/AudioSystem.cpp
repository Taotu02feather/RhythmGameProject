#include "AudioSystem.h"
#include <iostream>

namespace Ore {

// ============================================================================
// 构造函数 - 默认构造，音频设备尚未初始化
// ============================================================================
AudioSystem::AudioSystem() = default;

// ============================================================================
// 析构函数 - 确保音频设备被关闭
// ============================================================================
AudioSystem::~AudioSystem() {
    Shutdown();
}

// ============================================================================
// Initialize - 初始化 SDL_mixer 音频设备
//
// 初始化流程:
//   1. Mix_Init: 加载 MP3 和 OGG 解码器
//   2. Mix_OpenAudio: 打开音频设备，设置采样率/声道/缓冲
//   3. Mix_AllocateChannels: 预分配 16 个音效通道
//
// @param config: 音频参数配置
// @return true: 初始化成功
// @return false: 音频设备打开失败（游戏可继续无声运行）
// ============================================================================
bool AudioSystem::Initialize(const AudioConfig& config) {
    // ---- 加载音频解码器 ----
    // MIX_INIT_MP3: 支持 MP3 格式
    // MIX_INIT_OGG: 支持 OGG Vorbis 格式
    int flags = MIX_INIT_MP3 | MIX_INIT_OGG;
    int initted = Mix_Init(flags);
    if ((initted & flags) != flags) {
        std::cerr << "Mix_Init failed: " << Mix_GetError() << std::endl;
        std::cerr << "Audio init flags missing. MP3/OGG support may be limited." << std::endl;
    }

    // ---- 打开音频设备 ----
    // MIX_DEFAULT_FORMAT: 使用系统默认音频格式
    // channels: 声道数（2=立体声）
    // chunkSize: 缓冲区大小（越小延迟越低，但 CPU 开销越大）
    if (Mix_OpenAudio(config.sampleRate, MIX_DEFAULT_FORMAT, config.channels, config.chunkSize) < 0) {
        std::cerr << "Mix_OpenAudio failed: " << Mix_GetError() << std::endl;
        return false;
    }

    // ---- 预分配音效通道 ----
    // 16 个通道意味着可以同时播放 16 个音效
    Mix_AllocateChannels(16);
    m_initialized = true;

    std::cout << "AudioSystem initialized ("
              << config.sampleRate << " Hz, "
              << config.channels << " channels)." << std::endl;
    return true;
}

// ============================================================================
// Shutdown - 关闭音频设备，释放所有 SDL_mixer 资源
// ============================================================================
void AudioSystem::Shutdown() {
    if (m_initialized) {
        Mix_CloseAudio();    // 关闭音频设备
        Mix_Quit();          // 清理 SDL_mixer
        m_initialized = false;
    }
}

// ============================================================================
// LoadMusic - 从文件加载背景音乐
// 使用流式解码，适合长时间音频
// @param filepath: 音频文件路径
// @return Mix_Music 指针，失败返回 nullptr
// ============================================================================
Mix_Music* AudioSystem::LoadMusic(const std::string& filepath) {
    if (!m_initialized) return nullptr;
    Mix_Music* music = Mix_LoadMUS(filepath.c_str());
    if (!music) {
        std::cerr << "Failed to load music: " << filepath << " - " << Mix_GetError() << std::endl;
    }
    return music;
}

// ============================================================================
// PlayMusic - 播放背景音乐（同一时间只能播放一首）
// @param music: 已加载的音乐
// @param loops: 循环次数，0=播放一次，-1=无限循环
// ============================================================================
void AudioSystem::PlayMusic(Mix_Music* music, int loops) {
    if (!m_initialized || !music) return;
    if (Mix_PlayMusic(music, loops) < 0) {
        std::cerr << "Mix_PlayMusic failed: " << Mix_GetError() << std::endl;
    }
}

// ============================================================================
// StopMusic - 立即停止音乐播放
// ============================================================================
void AudioSystem::StopMusic() {
    Mix_HaltMusic();
}

// ============================================================================
// PauseMusic - 暂停音乐（可从当前位置恢复）
// ============================================================================
void AudioSystem::PauseMusic() {
    Mix_PauseMusic();
}

// ============================================================================
// ResumeMusic - 从暂停位置恢复播放
// ============================================================================
void AudioSystem::ResumeMusic() {
    Mix_ResumeMusic();
}

// ============================================================================
// IsMusicPlaying - 检查音乐是否正在播放
// ============================================================================
bool AudioSystem::IsMusicPlaying() const {
    return Mix_PlayingMusic() != 0;
}

// ============================================================================
// LoadSound - 从文件加载音效（全部加载到内存）
// 仅支持 WAV 格式（如需 MP3 音效，请先转换为 WAV）
// @param filepath: WAV 文件路径
// @return Mix_Chunk 指针，失败返回 nullptr
// ============================================================================
Mix_Chunk* AudioSystem::LoadSound(const std::string& filepath) {
    if (!m_initialized) return nullptr;
    Mix_Chunk* sound = Mix_LoadWAV(filepath.c_str());
    if (!sound) {
        std::cerr << "Failed to load sound: " << filepath << " - " << Mix_GetError() << std::endl;
    }
    return sound;
}

// ============================================================================
// PlaySound - 播放音效
// -1 表示自动分配到空闲通道，最多同时播放 16 个音效
// @param sound: 已加载的音效
// @param loops: 循环次数
// @return 播放通道编号
// ============================================================================
int AudioSystem::PlaySound(Mix_Chunk* sound, int loops) {
    if (!m_initialized || !sound) return -1;
    int channel = Mix_PlayChannel(-1, sound, loops);  // -1 = 自动选择空闲通道
    if (channel < 0) {
        std::cerr << "Mix_PlayChannel failed: " << Mix_GetError() << std::endl;
    }
    return channel;
}

// ============================================================================
// SetMusicVolume - 设置音乐音量
// @param volume: 0（静音）到 128（最大），SDL_mixer 标准范围
// ============================================================================
void AudioSystem::SetMusicVolume(int volume) {
    Mix_VolumeMusic(volume);
}

// ============================================================================
// SetSoundVolume - 设置所有音效通道的音量
// @param volume: 0（静音）到 128（最大）
// ============================================================================
void AudioSystem::SetSoundVolume(int volume) {
    Mix_Volume(-1, volume);  // -1 = 设置所有通道
}

// ============================================================================
// FreeMusic - 释放音乐占用的内存
// @param music: 要释放的 Mix_Music 指针
// ============================================================================
void AudioSystem::FreeMusic(Mix_Music* music) {
    if (music) Mix_FreeMusic(music);
}

// ============================================================================
// FreeSound - 释放音效占用的内存
// @param sound: 要释放的 Mix_Chunk 指针
// ============================================================================
void AudioSystem::FreeSound(Mix_Chunk* sound) {
    if (sound) Mix_FreeChunk(sound);
}

} // namespace Ore
