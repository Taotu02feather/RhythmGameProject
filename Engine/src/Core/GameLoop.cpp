#include "GameLoop.h"
#include "Engine.h"
#include "Renderer.h"
#include "Input.h"

#include <SDL.h>
#include <iostream>

namespace Ore {

// ============================================================================
// 构造函数 - 保存引擎指针
// @param engine: 引擎主控实例指针（用于访问渲染器/输入等子系统）
// ============================================================================
GameLoop::GameLoop(Engine* engine)
    : m_engine(engine)
{
}

// ============================================================================
// Run - 启动并运行主游戏循环（阻塞调用）
//
// 每帧执行流程:
//   1. 计算 deltaTime（使用 SDL 高精度性能计数器）
//   2. 处理 SDL 事件队列（窗口关闭、键盘输入等）
//   3. 更新输入状态（按键按下/释放检测）
//   4. 检查退出条件（窗口关闭 或 按键退出）
//   5. 调用 onUpdate 回调（业务逻辑更新）
//   6. 调用 onRender 回调（画面渲染）
//   7. 交换前后缓冲区（SDL_RenderPresent）
//   8. 重置每帧输入状态（准备下一帧）
//
// 防止帧率失控:
//   - deltaTime 超过 0.1秒时强制截断（防止调试断点后的"死亡螺旋"）
// ============================================================================
void GameLoop::Run() {
    // 获取子系统引用
    auto* renderer = m_engine->GetRenderer();
    auto* input = m_engine->GetInput();

    // ---- 初始化高精度计时器 ----
    // SDL_GetPerformanceCounter: 获取当前性能计数器值（纳秒级精度）
    // SDL_GetPerformanceFrequency: 获取计数器频率（每秒计数次数）
    uint64_t lastTime = SDL_GetPerformanceCounter();
    uint64_t perfFreq = SDL_GetPerformanceFrequency();

    std::cout << "Game loop started." << std::endl;

    // ---- 主循环 ----
    while (m_engine->IsRunning()) {

        // ---------- 1. 计算 deltaTime ----------
        uint64_t currentTime = SDL_GetPerformanceCounter();
        m_deltaTime = static_cast<double>(currentTime - lastTime) / static_cast<double>(perfFreq);
        lastTime = currentTime;
        m_frameCount++;

        if (m_deltaTime > 0.1) {
            m_deltaTime = 0.1;
        }

        // ---------- 2. 保存上一帧状态（必须在 PollEvent 之前） ----------
        // 核心: Update() 将当前 m_keyboardState 复制到 m_prevKeyboardState
        // 必须在 SDL_PollEvent 之前调用，这样:
        //   - m_prevKeyboardState = 帧开始时状态（上一帧结束后）
        //   - SDL_PollEvent 内部更新 m_keyboardState 为最新状态
        //   - IsKeyPressed() 比较二者即可检测边缘触发
        input->Update();

        // ---------- 3. 处理 SDL 事件（更新当前键盘状态） ----------
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    m_engine->Quit();
                    break;
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    break;
                default:
                    break;
            }
        }

        // ---------- 4. 检查退出条件 ----------
        if (input->IsQuitRequested()) {
            m_engine->Quit();
        }

        // ---------- 5. 逻辑更新回调 ----------
        if (m_onUpdate) {
            m_onUpdate(m_deltaTime);
        }

        // ---------- 6. 渲染 ----------
        renderer->BeginFrame();
        renderer->ClearScreen(20, 20, 30);

        if (m_onRender) {
            m_onRender(m_deltaTime);
        }

        renderer->EndFrame();

        // ---------- 7. 重置每帧状态 ----------
        input->ResetFrameState();
    }

    std::cout << "Game loop ended after " << m_frameCount << " frames." << std::endl;
}

} // namespace Ore
