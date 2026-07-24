#include "Renderer.h"
#include <iostream>

namespace Ore {

Renderer::Renderer() = default;

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Initialize(SDL_Window* window) {
    if (!window) {
        std::cerr << "Renderer: Invalid window pointer" << std::endl;
        return false;
    }

    m_sdlRenderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_sdlRenderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        return false;
    }

    std::cout << "Renderer initialized successfully." << std::endl;
    return true;
}

void Renderer::Shutdown() {
    if (m_sdlRenderer) {
        SDL_DestroyRenderer(m_sdlRenderer);
        m_sdlRenderer = nullptr;
    }
}

void Renderer::BeginFrame() {
    // Nothing special needed
}

void Renderer::EndFrame() {
    SDL_RenderPresent(m_sdlRenderer);
}

void Renderer::ClearScreen(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    SDL_SetRenderDrawColor(m_sdlRenderer, r, g, b, a);
    SDL_RenderClear(m_sdlRenderer);
}

void Renderer::DrawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    SDL_SetRenderDrawColor(m_sdlRenderer, r, g, b, a);
    SDL_Rect rect = { x, y, w, h };
    SDL_RenderFillRect(m_sdlRenderer, &rect);
}

void Renderer::DrawText(const std::string& text, int x, int y) {
    // Placeholder: Text rendering requires SDL_ttf or font textures.
    // Will be implemented in phase 2 with proper font support.
    (void)text;
    (void)x;
    (void)y;
}

} // namespace Ore