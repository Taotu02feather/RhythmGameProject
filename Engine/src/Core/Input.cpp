#include "Input.h"
#include <cstring>
#include <iostream>

namespace Ore {

// ============================================================================
// 构造函数 - 获取 SDL 键盘状态指针，分配上一帧状态缓冲区
//
// SDL_GetKeyboardState: 获取指向 SDL 内部键盘状态数组的指针
//   这个指针在 SDL 初始化后一直有效，不需要手动释放
// m_prevKeyboardState: 手动分配缓冲区保存上一帧状态
//   用于检测按键的"按下边缘"和"释放边缘"
// ============================================================================
Input::Input() {
    m_keyboardState = SDL_GetKeyboardState(&m_numKeys);  // 获取 SDL 键盘状态（不拥有所有权）
    m_prevKeyboardState = new uint8_t[m_numKeys];         // 分配上一帧状态缓冲区
    std::memset(m_prevKeyboardState, 0, m_numKeys);       // 初始化为全 0（所有按键未按下）
}

// ============================================================================
// Update - 更新键盘状态快照
//
// 保存当前帧的键盘状态到 m_prevKeyboardState，
// 然后 SDL 会在后续的 SDL_PollEvent 中自动更新 m_keyboardState。
// 通过比较当前帧和上一帧状态，实现边缘触发检测。
//
// 调用时机: 每帧开始时（在 SDL_PollEvent 之后）
// ============================================================================
void Input::Update() {
    // 保存当前状态到"上一帧"缓冲区
    std::memcpy(m_prevKeyboardState, m_keyboardState, m_numKeys);

    // SDL 内部会在 SDL_PollEvent 时自动更新 m_keyboardState，
    // 这里不需要手动更新
}

// ============================================================================
// ResetFrameState - 重置每帧状态（每帧结束时调用）
// 当前版本不需要额外操作，状态在下一帧 Update() 时自动更新
// ============================================================================
void Input::ResetFrameState() {
    // 不需要额外操作，下一帧 Update 会自动覆盖 m_prevKeyboardState
}

// ============================================================================
// IsKeyDown - 水平触发：按键是否当前被按住
// @param sc: SDL 扫描码
// @return true 表示按键处于按下状态
// ============================================================================
bool Input::IsKeyDown(SDL_Scancode sc) const {
    return m_keyboardState[sc] != 0;
}

// ============================================================================
// IsKeyPressed - 边缘触发：按键是否在本帧刚被按下
//
// 检测条件: 当前帧按下（状态=1）且上一帧未按下（状态=0）
// 用途: 触发一次性操作，例如菜单选项、note 打击判定
//
// @param sc: SDL 扫描码
// @return true 仅在本帧按下瞬间返回，下一帧变 false（即使键还按着）
// ============================================================================
bool Input::IsKeyPressed(SDL_Scancode sc) const {
    return m_keyboardState[sc] != 0 && m_prevKeyboardState[sc] == 0;
}

// ============================================================================
// IsKeyReleased - 边缘触发：按键是否在本帧刚被释放
//
// 检测条件: 当前帧未按下（状态=0）且上一帧按下（状态=1）
// 用途: 检测 hold note 的释放时机
//
// @param sc: SDL 扫描码
// @return true 仅在本帧释放瞬间返回
// ============================================================================
bool Input::IsKeyReleased(SDL_Scancode sc) const {
    return m_keyboardState[sc] == 0 && m_prevKeyboardState[sc] != 0;
}

// ============================================================================
// IsActionDown - 检查游戏动作是否被触发（水平触发）
//
// 遍历所有按键绑定，查找是否有绑定了该动作的按键被按下。
// 一个动作可以绑定多个按键（例如 Lane0 可以同时绑定 D 键和左方向键）
//
// @param action: 游戏虚拟动作
// @return true 表示该动作对应的按键正在被按住
// ============================================================================
bool Input::IsActionDown(GameAction action) const {
    for (const auto& [sc, act] : m_keyBindings) {
        if (act == action && IsKeyDown(sc)) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// IsActionPressed - 检查游戏动作是否在本帧刚被触发（边缘触发）
// @param action: 游戏虚拟动作
// ============================================================================
bool Input::IsActionPressed(GameAction action) const {
    for (const auto& [sc, act] : m_keyBindings) {
        if (act == action && IsKeyPressed(sc)) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// IsActionReleased - 检查游戏动作是否在本帧刚被释放（边缘触发）
// @param action: 游戏虚拟动作
// ============================================================================
bool Input::IsActionReleased(GameAction action) const {
    for (const auto& [sc, act] : m_keyBindings) {
        if (act == action && IsKeyReleased(sc)) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// BindKey - 将物理按键绑定到游戏动作
// 如果该物理按键已有绑定，会被覆盖
// @param sc: SDL 扫描码
// @param action: 游戏虚拟动作
// ============================================================================
void Input::BindKey(SDL_Scancode sc, GameAction action) {
    m_keyBindings[sc] = action;
}

// ============================================================================
// UnbindKey - 取消物理按键的所有绑定
// @param sc: SDL 扫描码
// ============================================================================
void Input::UnbindKey(SDL_Scancode sc) {
    m_keyBindings.erase(sc);
}

// ============================================================================
// LoadDefaultBindings - 加载默认按键布局
//
// 默认布局 (DFJK - 4K 模式):
//   轨道0: D 键（左手中指）
//   轨道1: F 键（左手食指）
//   轨道2: J 键（右手食指）
//   轨道3: K 键（右手中指）
//
// 扩展轨道 (6K/7K/8K 模式):
//   轨道4: Space（拇指）
//   轨道5: E 键
//   轨道6: I 键
//   轨道7: R 键
//
// 功能键:
//   ESC: 暂停 / 退出
//   Enter: 确认
//   Backspace: 取消
// ============================================================================
void Input::LoadDefaultBindings() {
    m_keyBindings.clear();

    // ---- 功能键 ----
    BindKey(SDL_SCANCODE_ESCAPE, GameAction::Pause);      // ESC = 暂停/退出
    BindKey(SDL_SCANCODE_RETURN, GameAction::Confirm);     // 回车 = 确认
    BindKey(SDL_SCANCODE_BACKSPACE, GameAction::Cancel);    // 退格 = 取消

    // ---- 轨道按键（A-L 行布局，从左到右依次为轨道 0-7） ----
    // 使用键盘第三行（左手区到右手区）的连续 8 个键
    BindKey(SDL_SCANCODE_A, GameAction::Lane0);             // A = 轨道0
    BindKey(SDL_SCANCODE_S, GameAction::Lane1);             // S = 轨道1
    BindKey(SDL_SCANCODE_D, GameAction::Lane2);             // D = 轨道2
    BindKey(SDL_SCANCODE_F, GameAction::Lane3);             // F = 轨道3
    BindKey(SDL_SCANCODE_G, GameAction::Lane4);             // G = 轨道4
    BindKey(SDL_SCANCODE_H, GameAction::Lane5);             // H = 轨道5
    BindKey(SDL_SCANCODE_J, GameAction::Lane6);             // J = 轨道6
    BindKey(SDL_SCANCODE_K, GameAction::Lane7);             // K = 轨道7

    std::cout << "Default input bindings loaded (A-K layout)." << std::endl;
}

} // namespace Ore
