#include "Renderer.h"
#include <iostream>

namespace Ore {

// ============================================================================
// 构造函数 - 默认构造，渲染器尚未初始化
// ============================================================================
Renderer::Renderer() = default;

// ============================================================================
// 析构函数 - 确保 SDL 渲染器被销毁
// ============================================================================
Renderer::~Renderer() {
    Shutdown();
}

// ============================================================================
// Initialize - 基于已有窗口创建 SDL 硬件加速渲染器
// @param window: SDL_Window 指针，必须已成功创建
// @return true: 渲染器创建成功
//
// 使用标志:
//   SDL_RENDERER_ACCELERATED: 使用 GPU 硬件加速
//   SDL_RENDERER_PRESENTVSYNC: 等待垂直同步，防止画面撕裂
// ============================================================================
bool Renderer::Initialize(SDL_Window* window) {
    if (!window) {
        std::cerr << "Renderer: Invalid window pointer" << std::endl;
        return false;
    }

    // -1 表示使用第一个支持所有标志的渲染驱动
    m_sdlRenderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_sdlRenderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        return false;
    }

    std::cout << "Renderer initialized successfully." << std::endl;
    return true;
}

// ============================================================================
// Shutdown - 销毁 SDL 渲染器，释放 GPU 资源
// ============================================================================
void Renderer::Shutdown() {
    if (m_sdlRenderer) {
        SDL_DestroyRenderer(m_sdlRenderer);
        m_sdlRenderer = nullptr;
    }
}

// ============================================================================
// BeginFrame - 标记新一帧渲染开始
// 当前版本不需要特殊操作（未来可能用于状态重置）
// ============================================================================
void Renderer::BeginFrame() {
    // 当前不需要特殊操作
}

// ============================================================================
// EndFrame - 交换缓冲区并显示渲染结果
// 调用 SDL_RenderPresent 将后台缓冲区内容显示到屏幕
// ============================================================================
void Renderer::EndFrame() {
    SDL_RenderPresent(m_sdlRenderer);
}

// ============================================================================
// ClearScreen - 用纯色清除整个屏幕
// @param r, g, b, a: RGBA 颜色分量（0-255），默认黑色不透明
// ============================================================================
void Renderer::ClearScreen(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    SDL_SetRenderDrawColor(m_sdlRenderer, r, g, b, a);
    SDL_RenderClear(m_sdlRenderer);
}

// ============================================================================
// DrawRect - 绘制填充矩形
// @param x, y: 矩形左上角坐标（像素）
// @param w, h: 矩形宽度和高度（像素）
// @param r, g, b, a: RGBA 颜色分量（0-255）
// ============================================================================
void Renderer::DrawRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    SDL_SetRenderDrawColor(m_sdlRenderer, r, g, b, a);
    SDL_Rect rect = { x, y, w, h };
    SDL_RenderFillRect(m_sdlRenderer, &rect);
}

// ============================================================================
// DrawText - 绘制文字（占位实现）
//
// 当前为占位，因为文字渲染需要 SDL_ttf 库。
// 将在 Phase 2（核心玩法阶段）集成 SDL_ttf 后实现真实文字渲染。
// 参数 (void) 避免编译器未使用参数警告。
// ============================================================================
void Renderer::DrawText(const std::string& text, int x, int y) {
    // 占位实现：等待 SDL_ttf 集成
    // SDL_ttf 集成后将使用 TTF_RenderText_Solid 等 API 渲染文字
    (void)text;
    (void)x;
    (void)y;
}

} // namespace Ore
