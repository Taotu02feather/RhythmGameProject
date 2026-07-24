#include "Engine.h"
#include "GameLoop.h"
#include "Renderer.h"
#include "Input.h"
#include "Audio/AudioSystem.h"
#include "Resource/ResourceManager.h"
#include "Chart/ChartLoader.h"

#include <SDL.h>
#include <iostream>

namespace Ore {

// ============================================================================
// 构造函数 - 保存配置参数
// @param config: 引擎配置（窗口标题、分辨率等）
// ============================================================================
Engine::Engine(const EngineConfig& config)
    : m_config(config)
{
}

// ============================================================================
// 析构函数 - 确保所有资源被释放
// ============================================================================
Engine::~Engine() {
    Shutdown();
}

// ============================================================================
// Initialize - 初始化 SDL 和所有子系统
//
// 初始化流程（按依赖顺序执行）：
//   第1步 - SDL_Init: 初始化 SDL 视频、音频、定时器子系统
//   第2步 - 创建所有子系统实例（unique_ptr 安全内存管理）
//   第3步 - ResourceManager::Init: 确定资源根目录（必须最先初始化）
//   第4步 - SDL_CreateWindow: 创建游戏窗口
//   第5步 - Renderer::Init: 创建 SDL 硬件加速渲染器
//   第6步 - AudioSystem::Init: 初始化音频设备（失败不致命，可无声运行）
//   第7步 - Input::LoadDefaultBindings: 加载默认按键绑定
//
// @return true: 核心模块全部初始化成功
// @return false: 关键模块（视频/窗口/渲染）初始化失败
// ============================================================================
bool Engine::Initialize() {
    // ---- 第1步：初始化 SDL 核心子系统 ----
    // SDL_INIT_VIDEO: 视频播放和窗口管理
    // SDL_INIT_AUDIO: 音频播放
    // SDL_INIT_TIMER: 高精度计时器（用于计算帧间 delta time）
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // ---- 第2步：创建所有子系统实例 ----
    // std::make_unique 确保异常安全的内存分配
    m_renderer = std::make_unique<Renderer>();
    m_input = std::make_unique<Input>();
    m_audio = std::make_unique<AudioSystem>();
    m_resourceManager = std::make_unique<ResourceManager>();
    // ChartLoader 依赖 ResourceManager，所以传入指针
    m_chartLoader = std::make_unique<ChartLoader>(m_resourceManager.get());
    // GameLoop 依赖 Engine 本身，传入 this 指针
    m_gameLoop = std::make_unique<GameLoop>(this);

    // ---- 第3步：初始化资源管理器（必须最先执行） ----
    // ResourceManager 确定文件系统的根目录，其他子系统可能依赖它
    if (!m_resourceManager->Initialize()) {
        std::cerr << "Failed to initialize ResourceManager" << std::endl;
        return false;
    }

    // ---- 第4步：创建 SDL 窗口 ----
    uint32_t windowFlags = SDL_WINDOW_SHOWN;  // 显示窗口
    if (m_config.fullscreen) {
        windowFlags |= SDL_WINDOW_FULLSCREEN;  // 全屏模式标志
    }

    SDL_Window* window = SDL_CreateWindow(
        m_config.windowTitle.c_str(),     // 窗口标题（如 "Open Rhythm Engine v0.1.0"）
        SDL_WINDOWPOS_CENTERED,           // X: 屏幕水平居中
        SDL_WINDOWPOS_CENTERED,           // Y: 屏幕垂直居中
        m_config.windowWidth,             // 宽度（默认 1280px）
        m_config.windowHeight,            // 高度（默认 720px）
        windowFlags                       // 窗口标志
    );

    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // ---- 第5步：初始化渲染器 ----
    // 创建 SDL 硬件加速渲染器，支持 VSync 防止画面撕裂
    if (!m_renderer->Initialize(window)) {
        std::cerr << "Failed to initialize Renderer" << std::endl;
        return false;
    }

    // ---- 第6步：初始化音频系统 ----
    // 音频初始化失败不会导致程序退出（允许无声运行）
    // 这对开发和测试阶段非常有用
    if (!m_audio->Initialize()) {
        std::cerr << "Failed to initialize AudioSystem" << std::endl;
        // 音频失败不致命，继续运行
    }

    // ---- 第7步：加载默认按键绑定 ----
    // 设置默认的键盘映射（DFJK 布局用于 4K 模式）
    m_input->LoadDefaultBindings();

    m_isRunning = true;
    std::cout << "Open Rhythm Engine initialized successfully." << std::endl;
    return true;
}

// ============================================================================
// Run - 进入主游戏循环
//
// 这是一个阻塞调用：程序会一直循环处理输入、更新逻辑、渲染画面，
// 直到用户关闭窗口或按下退出键（ESC）。
// 必须在 Initialize() 成功后才能调用。
// ============================================================================
void Engine::Run() {
    if (!m_isRunning) {
        std::cerr << "Engine not initialized. Call Initialize() first." << std::endl;
        return;
    }

    m_gameLoop->Run();
}

// ============================================================================
// Shutdown - 关闭引擎，按逆序释放所有资源
//
// 关闭顺序遵循"后创建先销毁"原则，确保依赖关系不被破坏：
//   1. 停止 GameLoop（不再处理帧更新）
//   2. 释放 ChartLoader
//   3. 关闭 AudioSystem → 释放音频设备
//   4. 关闭 Renderer → 销毁 SDL 渲染器和窗口
//   5. 释放 Input
//   6. 关闭 ResourceManager
//   7. SDL_Quit → 清理 SDL 所有全局状态
// ============================================================================
void Engine::Shutdown() {
    m_isRunning = false;

    // 按创建的逆序逐个释放子系统
    m_gameLoop.reset();               // 1. 停止游戏循环
    m_chartLoader.reset();            // 2. 释放谱面加载器
    m_audio->Shutdown();              // 3. 关闭音频设备
    m_audio.reset();                  // 4. 释放音频系统
    m_renderer->Shutdown();           // 5. 销毁 SDL 渲染器
    m_renderer.reset();               // 6. 释放渲染器
    m_input.reset();                  // 7. 释放输入管理器
    m_resourceManager->Shutdown();    // 8. 关闭资源管理器
    m_resourceManager.reset();        // 9. 释放资源管理器

    SDL_Quit();                       // 10. 清理 SDL 所有资源
    std::cout << "Open Rhythm Engine shut down." << std::endl;
}

} // namespace Ore
