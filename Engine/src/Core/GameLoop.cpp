#include "GameLoop.h"
#include "Engine.h"
#include "Renderer.h"
#include "Input.h"

#include <SDL.h>
#include <iostream>

namespace Ore {

GameLoop::GameLoop(Engine* engine)
    : m_engine(engine)
{
}

void GameLoop::Run() {
    auto* renderer = m_engine->GetRenderer();
    auto* input = m_engine->GetInput();

    uint64_t lastTime = SDL_GetPerformanceCounter();
    uint64_t perfFreq = SDL_GetPerformanceFrequency();

    std::cout << "Game loop started." << std::endl;

    while (m_engine->IsRunning()) {
        // Calculate delta time
        uint64_t currentTime = SDL_GetPerformanceCounter();
        m_deltaTime = static_cast<double>(currentTime - lastTime) / static_cast<double>(perfFreq);
        lastTime = currentTime;
        m_frameCount++;

        // Cap delta time to avoid spiral of death
        if (m_deltaTime > 0.1) {
            m_deltaTime = 0.1;
        }

        // Process all SDL events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    m_engine->Quit();
                    break;
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    // Handled by Input::Update below
                    break;
                default:
                    break;
            }
        }

        // Update input state
        input->Update();

        // Check quit request from input (e.g., Escape key)
        if (input->IsQuitRequested()) {
            m_engine->Quit();
        }

        // Custom update callback
        if (m_onUpdate) {
            m_onUpdate(m_deltaTime);
        }

        // Render
        renderer->BeginFrame();
        renderer->ClearScreen(20, 20, 30);  // Dark background

        if (m_onRender) {
            m_onRender(m_deltaTime);
        }

        renderer->EndFrame();

        // Reset per-frame input state
        input->ResetFrameState();
    }

    std::cout << "Game loop ended after " << m_frameCount << " frames." << std::endl;
}

} // namespace Ore