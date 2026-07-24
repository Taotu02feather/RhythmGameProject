#pragma once

#include <SDL.h>
#include <string>
#include <cstdint>

namespace Ore {

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Initialize(SDL_Window* window);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void ClearScreen(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255);
    void DrawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void DrawText(const std::string& text, int x, int y);

    SDL_Renderer* GetSDLRenderer() const { return m_sdlRenderer; }

private:
    SDL_Renderer* m_sdlRenderer = nullptr;
};

} // namespace Ore