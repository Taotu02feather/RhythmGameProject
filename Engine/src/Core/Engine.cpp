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

Engine::Engine(const EngineConfig& config)
    : m_config(config)
{
}

Engine::~Engine() {
    Shutdown();
}

bool Engine::Initialize() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create subsystems
    m_renderer = std::make_unique<Renderer>();
    m_input = std::make_unique<Input>();
    m_audio = std::make_unique<AudioSystem>();
    m_resourceManager = std::make_unique<ResourceManager>();
    m_chartLoader = std::make_unique<ChartLoader>(m_resourceManager.get());
    m_gameLoop = std::make_unique<GameLoop>(this);

    // Initialize ResourceManager (must be first as others may depend on it)
    if (!m_resourceManager->Initialize()) {
        std::cerr << "Failed to initialize ResourceManager" << std::endl;
        return false;
    }

    // Create SDL window
    uint32_t windowFlags = SDL_WINDOW_SHOWN;
    if (m_config.fullscreen) {
        windowFlags |= SDL_WINDOW_FULLSCREEN;
    }

    SDL_Window* window = SDL_CreateWindow(
        m_config.windowTitle.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        m_config.windowWidth,
        m_config.windowHeight,
        windowFlags
    );

    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // Initialize Renderer
    if (!m_renderer->Initialize(window)) {
        std::cerr << "Failed to initialize Renderer" << std::endl;
        return false;
    }

    // Initialize Audio
    if (!m_audio->Initialize()) {
        std::cerr << "Failed to initialize AudioSystem" << std::endl;
        // Audio failure is non-critical, continue without audio
    }

    // Setup default input bindings
    m_input->LoadDefaultBindings();

    m_isRunning = true;
    std::cout << "Open Rhythm Engine initialized successfully." << std::endl;
    return true;
}

void Engine::Run() {
    if (!m_isRunning) {
        std::cerr << "Engine not initialized. Call Initialize() first." << std::endl;
        return;
    }

    m_gameLoop->Run();
}

void Engine::Shutdown() {
    m_isRunning = false;

    // Shutdown in reverse order
    m_gameLoop.reset();
    m_chartLoader.reset();
    m_audio->Shutdown();
    m_audio.reset();
    m_renderer->Shutdown();
    m_renderer.reset();
    m_input.reset();
    m_resourceManager->Shutdown();
    m_resourceManager.reset();

    SDL_Quit();
    std::cout << "Open Rhythm Engine shut down." << std::endl;
}

} // namespace Ore