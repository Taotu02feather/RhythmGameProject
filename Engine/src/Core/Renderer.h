#pragma once

#include <SDL.h>
#include <string>
#include <cstdint>

namespace Ore {

// ============================================================================
// Renderer - SDL2 渲染器封装
//
// 职责:
//   1. 封装 SDL_Renderer 的创建/销毁
//   2. 提供清屏、绘制矩形、绘制文字等基础绘制接口
//   3. 管理帧开始/结束（BeginFrame / EndFrame）
//
// 当前版本使用 SDL2 内置渲染（SDL_RenderFillRect），
// 未来可以升级为 OpenGL/Vulkan 后端，接口不变
// ============================================================================
class Renderer {
public:
    Renderer();
    ~Renderer();

    // ---------- 生命周期 ----------

    // Initialize - 基于已有窗口创建 SDL 渲染器
    // @param window: SDL_Window 指针，渲染器将绑定到此窗口
    // @return true: 创建成功
    // 使用硬件加速 + VSync 标志
    bool Initialize(SDL_Window* window);

    // Shutdown - 销毁 SDL 渲染器，释放 GPU 资源
    void Shutdown();

    // ---------- 帧控制 ----------

    // BeginFrame - 标记新一帧渲染开始
    void BeginFrame();

    // EndFrame - 交换前后缓冲区，将渲染结果显示到屏幕
    void EndFrame();

    // ---------- 绘制接口 ----------

    // ClearScreen - 用纯色填充整个屏幕
    // @param r, g, b, a: RGBA 颜色分量（0-255）
    void ClearScreen(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255);

    // DrawRect - 绘制填充矩形
    // @param x, y: 左上角坐标
    // @param w, h: 宽度和高度
    // @param r, g, b, a: RGBA 颜色分量（0-255）
    void DrawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    // DrawText - 绘制文字（当前为占位实现，等待 SDL_ttf 集成）
    // @param text: 要绘制的字符串
    // @param x, y: 绘制坐标
    void DrawText(const std::string& text, int x, int y);

    // ---------- 访问器 ----------

    // GetSDLRenderer - 获取底层 SDL_Renderer 指针
    // 仅在需要直接调用 SDL API 时使用
    SDL_Renderer* GetSDLRenderer() const { return m_sdlRenderer; }

private:
    SDL_Renderer* m_sdlRenderer = nullptr;  // SDL 渲染器指针
};

} // namespace Ore
