#pragma once

#include <SDL.h>
#include <cstdint>
#include <unordered_map>

namespace Ore {

// Virtual key codes for game actions (key binding friendly)
enum class GameAction {
    Confirm,
    Cancel,
    Pause,

    // Lane keys - up to 8 lanes, mapped by index
    Lane0,
    Lane1,
    Lane2,
    Lane3,
    Lane4,
    Lane5,
    Lane6,
    Lane7,

    COUNT
};

class Input {
public:
    Input();
    ~Input() = default;

    void Update();
    void ResetFrameState();

    // Raw keyboard state
    bool IsKeyDown(SDL_Scancode sc) const;
    bool IsKeyPressed(SDL_Scancode sc) const;   // true only on the frame it was pressed
    bool IsKeyReleased(SDL_Scancode sc) const;  // true only on the frame it was released

    // Game action state
    bool IsActionDown(GameAction action) const;
    bool IsActionPressed(GameAction action) const;
    bool IsActionReleased(GameAction action) const;

    // Binding
    void BindKey(SDL_Scancode sc, GameAction action);
    void UnbindKey(SDL_Scancode sc);

    // Default bindings for common layouts
    void LoadDefaultBindings();

    // Quit event
    bool IsQuitRequested() const { return m_quitRequested; }

private:
    const uint8_t* m_keyboardState = nullptr;
    int m_numKeys = 0;

    // Previous frame state for edge detection
    uint8_t* m_prevKeyboardState = nullptr;

    // Key -> Action mapping
    std::unordered_map<SDL_Scancode, GameAction> m_keyBindings;

    bool m_quitRequested = false;
};

} // namespace Ore