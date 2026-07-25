#pragma once

#include <SDL.h>
#include <string>
#include <cstdint>

namespace Ore {

// ============================================================================
// Renderer - SDL2 渲染器封装（含内置像素字体）
// ============================================================================
class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Initialize(SDL_Window* window);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void ClearScreen(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255);

    // 绘制填充矩形
    void DrawRect(int x, int y, int w, int h,
                  uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    // ---------- 内置像素字体渲染 ----------
    // 绘制字符串（支持 ASCII 32-126 和部分常用中文）
    // @param text: UTF-8 编码字符串
    // @param x, y: 左上角坐标
    // @param scale: 缩放倍数（默认 2x = 每个像素点 2x2）
    // @param r, g, b, a: 颜色
    void DrawPixelText(const std::string& text, int x, int y,
                       uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, uint8_t a = 255,
                       int scale = 2);

    // 获取文字宽度（像素）
    int GetTextWidth(const std::string& text, int scale = 2);

    SDL_Renderer* GetSDLRenderer() const { return m_sdlRenderer; }

private:
    SDL_Renderer* m_sdlRenderer = nullptr;

    // 内置 5x7 像素字体 — 只包含 ASCII 32-126，不使用外部资源
    // 每个字符 5 宽 × 7 高，用 7 个 uint8_t 表示（每行一个字节，低 5 位有效）
    static const uint8_t* GetCharBitmap(char c);

    // 绘制单个字符
    void DrawPixelChar(char c, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a, int scale);
};

} // namespace Ore