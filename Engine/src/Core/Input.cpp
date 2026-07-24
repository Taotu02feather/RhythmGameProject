#include "Input.h"
#include <cstring>
#include <iostream>

namespace Ore {

Input::Input() {
    m_keyboardState = SDL_GetKeyboardState(&m_numKeys);
    m_prevKeyboardState = new uint8_t[m_numKeys];
    std::memset(m_prevKeyboardState, 0, m_numKeys);
}

void Input::Update() {
    // Save previous state
    std::memcpy(m_prevKeyboardState, m_keyboardState, m_numKeys);

    // SDL updates keyboard state internally via SDL_PumpEvents
    // (already called by SDL_PollEvent in GameLoop)
}

void Input::ResetFrameState() {
    // Called at end of frame to prepare for next frame
    // The previous state will be updated at the start of next Update()
}

bool Input::IsKeyDown(SDL_Scancode sc) const {
    return m_keyboardState[sc] != 0;
}

bool Input::IsKeyPressed(SDL_Scancode sc) const {
    return m_keyboardState[sc] != 0 && m_prevKeyboardState[sc] == 0;
}

bool Input::IsKeyReleased(SDL_Scancode sc) const {
    return m_keyboardState[sc] == 0 && m_prevKeyboardState[sc] != 0;
}

bool Input::IsActionDown(GameAction action) const {
    for (const auto& [sc, act] : m_keyBindings) {
        if (act == action && IsKeyDown(sc)) {
            return true;
        }
    }
    return false;
}

bool Input::IsActionPressed(GameAction action) const {
    for (const auto& [sc, act] : m_keyBindings) {
        if (act == action && IsKeyPressed(sc)) {
            return true;
        }
    }
    return false;
}

bool Input::IsActionReleased(GameAction action) const {
    for (const auto& [sc, act] : m_keyBindings) {
        if (act == action && IsKeyReleased(sc)) {
            return true;
        }
    }
    return false;
}

void Input::BindKey(SDL_Scancode sc, GameAction action) {
    m_keyBindings[sc] = action;
}

void Input::UnbindKey(SDL_Scancode sc) {
    m_keyBindings.erase(sc);
}

void Input::LoadDefaultBindings() {
    m_keyBindings.clear();

    // Default: Escape = quit, Enter = Confirm, Backspace = Cancel
    BindKey(SDL_SCANCODE_ESCAPE, GameAction::Pause);
    BindKey(SDL_SCANCODE_RETURN, GameAction::Confirm);
    BindKey(SDL_SCANCODE_BACKSPACE, GameAction::Cancel);

    // Default lane keys for up to 8K:
    // Lane0 = D, Lane1 = F, Lane2 = J, Lane3 = K (4K default)
    // Extended: Lane4 = Space, Lane5 = E, Lane6 = I, Lane7 = R
    BindKey(SDL_SCANCODE_D, GameAction::Lane0);
    BindKey(SDL_SCANCODE_F, GameAction::Lane1);
    BindKey(SDL_SCANCODE_J, GameAction::Lane2);
    BindKey(SDL_SCANCODE_K, GameAction::Lane3);
    BindKey(SDL_SCANCODE_SPACE, GameAction::Lane4);
    BindKey(SDL_SCANCODE_E, GameAction::Lane5);
    BindKey(SDL_SCANCODE_I, GameAction::Lane6);
    BindKey(SDL_SCANCODE_R, GameAction::Lane7);

    std::cout << "Default input bindings loaded (DFJK layout)." << std::endl;
}

} // namespace Ore