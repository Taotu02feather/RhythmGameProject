#pragma once

#include <string>
#include <memory>

namespace Ore {

// ============================================================================
// 前向声明 - 避免头文件循环依赖，加快编译速度
// ============================================================================
class GameLoop;
class Renderer;
class Input;
class AudioSystem;
class ResourceManager;
class ChartLoader;

// ============================================================================
// EngineConfig - 引擎初始化配置参数
// 用途: 在创建 Engine 实例前设置窗口标题、分辨率等参数
// ============================================================================
struct EngineConfig {
    std::string windowTitle = "Open Rhythm Engine";  // 窗口标题
    int windowWidth = 1280;                           // 窗口宽度（像素）
    int windowHeight = 720;                           // 窗口高度（像素）
    bool fullscreen = false;                          // 是否全屏模式
    bool vsync = true;                                // 是否垂直同步（防止画面撕裂）
};

// ============================================================================
// Engine - 游戏引擎主控类
//
// 职责:
//   1. 初始化 SDL 和所有子系统（渲染器、输入、音频、资源管理等）
//   2. 管理整个游戏生命周期：初始化 → 运行 → 关闭
//   3. 为外部代码（main.cpp）提供统一的子系统访问接口
//
// 设计原则:
//   - 单例模式：整个游戏只有一个 Engine 实例
//   - 依赖注入：所有子系统通过 Engine 获取，不直接创建
//   - 资源管理：使用 std::unique_ptr 自动管理子系统生命周期
// ============================================================================
class Engine {
public:
    // ---------- 构造与析构 ----------
    // @param config: 引擎初始化配置（窗口标题、分辨率等）
    explicit Engine(const EngineConfig& config);
    ~Engine();

    // 禁止拷贝（引擎实例应该是唯一的）
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // ---------- 生命周期 ----------

    // Initialize: 初始化 SDL、创建窗口、初始化所有子系统
    // @return true 表示全部初始化成功，false 表示失败
    bool Initialize();

    // Run: 进入主游戏循环，阻塞直到用户退出
    // 必须在 Initialize() 成功后调用
    void Run();

    // Shutdown: 按逆序关闭所有子系统，释放 SDL 资源
    void Shutdown();

    // ---------- 子系统访问器 ----------
    // 每个 Get* 函数返回对应子系统的裸指针
    // 调用者不能删除这些指针，生命周期由 Engine 管理

    Renderer* GetRenderer() const { return m_renderer.get(); }
    Input* GetInput() const { return m_input.get(); }
    AudioSystem* GetAudio() const { return m_audio.get(); }
    GameLoop* GetGameLoop() const { return m_gameLoop.get(); }
    ResourceManager* GetResourceManager() const { return m_resourceManager.get(); }
    ChartLoader* GetChartLoader() const { return m_chartLoader.get(); }

    // ---------- 运行状态 ----------
    // @return true 表示游戏循环正在运行
    bool IsRunning() const { return m_isRunning; }
    // 请求退出游戏循环（下一帧检测到后退出）
    void Quit() { m_isRunning = false; }

private:
    EngineConfig m_config;                                // 引擎配置
    bool m_isRunning = false;                             // 运行状态标志

    // 使用 unique_ptr 实现独占所有权，自动资源管理
    std::unique_ptr<GameLoop>    m_gameLoop;              // 游戏循环管理器
    std::unique_ptr<Renderer>    m_renderer;              // SDL2 渲染器
    std::unique_ptr<Input>       m_input;                 // 输入管理器
    std::unique_ptr<AudioSystem> m_audio;                 // 音频系统
    std::unique_ptr<ResourceManager> m_resourceManager;   // 资源管理器
    std::unique_ptr<ChartLoader> m_chartLoader;           // 谱面加载器
};

} // namespace Ore