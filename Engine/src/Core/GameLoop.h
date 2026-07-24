#pragma once

#include <cstdint>
#include <functional>

namespace Ore {

class Engine;  // 前向声明：避免循环依赖

// ============================================================================
// GameLoop - 游戏主循环管理器
//
// 职责:
//   1. 驱动"事件处理 → 逻辑更新 → 渲染"的帧循环
//   2. 计算并管理 deltaTime（帧间隔时间），用于帧率无关的逻辑
//   3. 提供回调机制：外部代码通过 SetOnUpdate/SetOnRender 注入每帧逻辑
//
// 设计模式: 回调模式（Callback Pattern）
//   引擎核心负责循环控制，业务逻辑通过回调函数注入
// ============================================================================
class GameLoop {
public:
    // 构造函数 - 保存引擎指针用于访问子系统
    // @param engine: 引擎主控实例指针（不会被 GameLoop 拥有或释放）
    explicit GameLoop(Engine* engine);
    ~GameLoop() = default;

    // Run - 启动主循环（阻塞调用）
    // 循环流程：处理SDL事件 → 计算deltaTime → 调用Update回调 → 调用Render回调 → 交换缓冲区
    void Run();

    // ---------- 配置 ----------

    // SetFixedTimestep - 设置固定更新步长（0 = 不限帧率）
    // @param dt: 固定步长（秒），设为0则使用可变帧率
    void SetFixedTimestep(double dt) { m_fixedDt = dt; }

    // ---------- 回调设置 ----------

    // SetOnUpdate - 设置每帧逻辑更新回调
    // @param callback: 回调函数，参数为 deltaTime（秒）
    void SetOnUpdate(std::function<void(double)> callback) { m_onUpdate = std::move(callback); }

    // SetOnRender - 设置每帧渲染回调
    // @param callback: 回调函数，参数为 deltaTime（秒）
    void SetOnRender(std::function<void(double)> callback) { m_onRender = std::move(callback); }

    // ---------- 帧信息访问器 ----------

    // GetDeltaTime - 获取当前帧的时间间隔（秒）
    double GetDeltaTime() const { return m_deltaTime; }

    // GetFrameCount - 获取从循环开始累计的帧数
    uint64_t GetFrameCount() const { return m_frameCount; }

private:
    Engine* m_engine;                       // 引擎指针（不拥有所有权）
    double m_fixedDt = 0.0;                 // 固定时间步长（0=不限帧率）
    double m_deltaTime = 0.0;               // 当前帧的时间间隔（秒）
    uint64_t m_frameCount = 0;              // 已处理的帧数

    std::function<void(double)> m_onUpdate; // 每帧逻辑更新回调
    std::function<void(double)> m_onRender; // 每帧渲染回调
};

} // namespace Ore
