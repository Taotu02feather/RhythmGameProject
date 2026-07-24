#pragma once

#include <cstdint>
#include <functional>

namespace Ore {

class Engine;

class GameLoop {
public:
    explicit GameLoop(Engine* engine);
    ~GameLoop() = default;

    void Run();

    // Set a fixed update rate (0 = unlimited)
    void SetFixedTimestep(double dt) { m_fixedDt = dt; }

    // Callback for custom per-frame logic (called after SDL events are processed)
    void SetOnUpdate(std::function<void(double)> callback) { m_onUpdate = std::move(callback); }
    void SetOnRender(std::function<void(double)> callback) { m_onRender = std::move(callback); }

    double GetDeltaTime() const { return m_deltaTime; }
    uint64_t GetFrameCount() const { return m_frameCount; }

private:
    Engine* m_engine;
    double m_fixedDt = 0.0;
    double m_deltaTime = 0.0;
    uint64_t m_frameCount = 0;

    std::function<void(double)> m_onUpdate;
    std::function<void(double)> m_onRender;
};

} // namespace Ore