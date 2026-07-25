#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
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
    void DrawRect(int x, int y, int w, int h,
                  uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    // ---------- SDL_ttf 文字渲染（中英文） ----------
    bool LoadFont(const std::string& path, int size = 24);
    void DrawTTFText(const std::string& text, int x, int y,
                     uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, uint8_t a = 255);
    int GetTTFTextWidth(const std::string& text);
    int TTFTextHeight();

    // ---------- 内置 5×7 像素字体（英文回退） ----------
    void DrawPixelText(const std::string& text, int x, int y,
                       uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, uint8_t a = 255,
                       int scale = 2);
    int GetPixelTextWidth(const std::string& text, int scale = 2);

    SDL_Renderer* GetSDLRenderer() const { return m_sdlRenderer; }

private:
    SDL_Renderer* m_sdlRenderer = nullptr;
    TTF_Font* m_font = nullptr;
    int m_fontSize = 24;

    static const uint8_t* GetCharBitmap(char c);
    void DrawPixelChar(char c, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a, int scale);
};

} // namespace Ore