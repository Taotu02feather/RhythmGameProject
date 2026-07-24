#pragma once

#include <SDL_mixer.h>
#include <string>
#include <memory>

namespace Ore {

// ============================================================================
// AudioConfig - 音频系统初始化配置
// 控制采样率、声道数、缓冲区大小等底层音频参数
// ============================================================================
struct AudioConfig {
    int sampleRate = 44100;   // 采样率（Hz）：44100 = CD 音质
    int channels = 2;         // 声道数：2 = 立体声
    int chunkSize = 2048;     // 音频缓冲区大小（字节）：越小延迟越低，但越容易爆音
};

// ============================================================================
// AudioSystem - 音频播放系统（基于 SDL2_mixer）
//
// 职责:
//   1. 初始化/关闭音频设备
//   2. 背景音乐播放（Music）：长时间音频，同一时刻只能播放一首
//   3. 音效播放（Chunk）：短音频，可同时播放多个（如打击音效）
//   4. 音量控制
//
// Music vs Chunk 区别:
//   - Music: 使用流式解码，适合 3-10 分钟的歌曲
//   - Chunk: 全部加载到内存，适合 < 5秒的音效（如打击音效、UI音效）
// ============================================================================
class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    // ---------- 生命周期 ----------

    // Initialize - 初始化 SDL_mixer 音频设备
    // @param config: 音频参数配置（采样率、声道数等）
    // @return true: 初始化成功
    // 注意: 初始化失败不致命，游戏可以无声运行
    bool Initialize(const AudioConfig& config = AudioConfig{});

    // Shutdown - 关闭音频设备，释放 SDL_mixer 资源
    void Shutdown();

    // ---------- 背景音乐播放 ----------

    // LoadMusic - 从文件加载音乐（支持 MP3/OGG/WAV）
    // @param filepath: 音频文件路径
    // @return Mix_Music 指针，失败返回 nullptr
    Mix_Music* LoadMusic(const std::string& filepath);

    // PlayMusic - 播放已加载的音乐
    // @param music: LoadMusic 返回的音乐指针
    // @param loops: 循环次数，0=播放一次，-1=无限循环
    void PlayMusic(Mix_Music* music, int loops = 0);

    // StopMusic - 停止当前音乐播放
    void StopMusic();

    // PauseMusic - 暂停当前音乐（可从暂停位置恢复）
    void PauseMusic();

    // ResumeMusic - 从暂停位置恢复音乐播放
    void ResumeMusic();

    // IsMusicPlaying - 检查音乐是否正在播放
    bool IsMusicPlaying() const;

    // ---------- 音效播放 ----------

    // LoadSound - 从文件加载音效（WAV 格式）
    // @param filepath: 音效文件路径
    // @return Mix_Chunk 指针，失败返回 nullptr
    Mix_Chunk* LoadSound(const std::string& filepath);

    // PlaySound - 播放音效
    // @param sound: LoadSound 返回的音效指针
    // @param loops: 循环次数，0=播放一次
    // @return 播放通道编号，-1 表示失败
    int PlaySound(Mix_Chunk* sound, int loops = 0);

    // ---------- 音量控制 ----------

    // SetMusicVolume - 设置音乐音量
    // @param volume: 音量值（0=静音, 128=最大）
    void SetMusicVolume(int volume);

    // SetSoundVolume - 设置音效音量
    // @param volume: 音量值（0=静音, 128=最大）
    void SetSoundVolume(int volume);

    // ---------- 资源释放 ----------

    // FreeMusic - 释放音乐资源
    void FreeMusic(Mix_Music* music);

    // FreeSound - 释放音效资源
    void FreeSound(Mix_Chunk* sound);

private:
    bool m_initialized = false;  // 音频设备是否已初始化
};

} // namespace Ore
