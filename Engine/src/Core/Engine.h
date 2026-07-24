#pragma once

#include <string>
#include <memory>

namespace Ore {

// Forward declarations
class GameLoop;
class Renderer;
class Input;
class AudioSystem;
class ResourceManager;
class ChartLoader;

struct EngineConfig {
    std::string windowTitle = "Open Rhythm Engine";
    int windowWidth = 1280;
    int windowHeight = 720;
    bool fullscreen = false;
    bool vsync = true;
};

class Engine {
public:
    Engine(const EngineConfig& config);
    ~Engine();

    // Non-copyable
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    bool Initialize();
    void Run();
    void Shutdown();

    // Accessors for subsystems
    Renderer* GetRenderer() const { return m_renderer.get(); }
    Input* GetInput() const { return m_input.get(); }
    AudioSystem* GetAudio() const { return m_audio.get(); }
    GameLoop* GetGameLoop() const { return m_gameLoop.get(); }
    ResourceManager* GetResourceManager() const { return m_resourceManager.get(); }
    ChartLoader* GetChartLoader() const { return m_chartLoader.get(); }

    bool IsRunning() const { return m_isRunning; }
    void Quit() { m_isRunning = false; }

private:
    EngineConfig m_config;
    bool m_isRunning = false;

    std::unique_ptr<GameLoop>    m_gameLoop;
    std::unique_ptr<Renderer>    m_renderer;
    std::unique_ptr<Input>       m_input;
    std::unique_ptr<AudioSystem> m_audio;
    std::unique_ptr<ResourceManager> m_resourceManager;
    std::unique_ptr<ChartLoader> m_chartLoader;
};

} // namespace Ore