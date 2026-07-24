#pragma once

#include <SDL.h>
#include <cstdint>
#include <unordered_map>

namespace Ore {

// ============================================================================
// GameAction - 游戏虚拟动作枚举
//
// 设计目的: 将物理按键与游戏逻辑解耦
//   玩家可以自由绑定任意按键到这些虚拟动作，
//   游戏逻辑只关心 GameAction，不关心具体按键。
//   支持最多 8 条轨道（2K-8K 模式）
// ============================================================================
enum class GameAction {
    Confirm,   // 确认操作（Enter 键默认）
    Cancel,    // 取消操作（Backspace 键默认）
    Pause,     // 暂停游戏（ESC 键默认）

    Lane0,     // 轨道 0 的按键
    Lane1,     // 轨道 1 的按键
    Lane2,     // 轨道 2 的按键
    Lane3,     // 轨道 3 的按键
    Lane4,     // 轨道 4 的按键（6K/7K/8K 模式扩展）
    Lane5,     // 轨道 5 的按键
    Lane6,     // 轨道 6 的按键
    Lane7,     // 轨道 7 的按键

    COUNT      // 枚举计数（用于遍历）
};

// ============================================================================
// Input - 输入管理器
//
// 职责:
//   1. 封装 SDL 键盘状态查询
//   2. 提供按键"按下"和"释放"的边缘触发检测
//   3. 将物理按键映射到游戏虚拟动作（GameAction）
//   4. 提供默认按键绑定（DFJK 布局）
//
// 边缘触发 vs 水平触发:
//   - IsKeyDown: 水平触发 - 只要按键保持按下就返回 true
//   - IsKeyPressed: 边缘触发 - 仅在按下瞬间返回 true（下一帧变 false）
//   - IsKeyReleased: 边缘触发 - 仅在释放瞬间返回 true
// ============================================================================
class Input {
public:
    // 构造函数 - 获取键盘状态指针
    Input();
    ~Input() = default;

    // ---------- 帧更新 ----------

    // Update - 更新键盘状态快照（必须在每帧开始时调用）
    // 保存当前帧状态并与上一帧比较，实现边缘检测
    void Update();

    // ResetFrameState - 重置每帧的按下/释放标志（每帧结束时调用）
    void ResetFrameState();

    // ---------- 原始键盘状态 ----------

    // IsKeyDown - 检查按键是否被按住（水平触发）
    // @param sc: SDL 扫描码（物理按键位置，不受键盘布局影响）
    bool IsKeyDown(SDL_Scancode sc) const;

    // IsKeyPressed - 检查按键是否在本帧刚被按下（边缘触发）
    // 常用于触发一次性操作（如菜单选择）
    bool IsKeyPressed(SDL_Scancode sc) const;

    // IsKeyReleased - 检查按键是否在本帧刚被释放（边缘触发）
    // 常用于检测按键松开的瞬间
    bool IsKeyReleased(SDL_Scancode sc) const;

    // ---------- 游戏动作状态 ----------

    // 以下三个函数与原始键盘状态函数对应，但使用 GameAction 而非物理扫描码
    // 游戏逻辑应优先使用这些函数，以支持自定义按键绑定

    bool IsActionDown(GameAction action) const;
    bool IsActionPressed(GameAction action) const;
    bool IsActionReleased(GameAction action) const;

    // ---------- 按键绑定 ----------

    // BindKey - 将物理按键绑定到游戏动作
    // @param sc: SDL 扫描码
    // @param action: 游戏虚拟动作
    void BindKey(SDL_Scancode sc, GameAction action);

    // UnbindKey - 取消物理按键的绑定
    void UnbindKey(SDL_Scancode sc);

    // LoadDefaultBindings - 加载默认按键绑定
    // 默认布局: DFJK（4K），扩展 E/R/Space/I 用于多轨
    void LoadDefaultBindings();

    // ---------- 退出检测 ----------

    // IsQuitRequested - 检查是否收到退出请求
    // 当前通过 ESC 键 = Pause 动作触发
    bool IsQuitRequested() const { return m_quitRequested; }

private:
    const uint8_t* m_keyboardState = nullptr;   // SDL 键盘状态数组（当前帧）
    int m_numKeys = 0;                           // 键盘按键总数

    uint8_t* m_prevKeyboardState = nullptr;      // 上一帧键盘状态（用于边缘检测）

    // 物理按键 → 游戏动作 映射表
    std::unordered_map<SDL_Scancode, GameAction> m_keyBindings;

    bool m_quitRequested = false;                // 退出请求标志
};

} // namespace Ore
