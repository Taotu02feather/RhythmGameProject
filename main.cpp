#include "Core/Engine.h"
#include "Core/Renderer.h"
#include "Core/Input.h"
#include "Chart/ChartLoader.h"
#include "Chart/ChartTypes.h"
#include "Gameplay/Judge.h"

#include <iostream>
#include <memory>

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "  Open Rhythm Engine v0.1.0" << std::endl;
    std::cout << "========================================" << std::endl;

    // ======== Engine Configuration ========
    Ore::EngineConfig config;
    config.windowTitle = "Open Rhythm Engine v0.1.0";
    config.windowWidth = 1280;
    config.windowHeight = 720;

    // ======== Create and Initialize Engine ========
    Ore::Engine engine(config);

    if (!engine.Initialize()) {
        std::cerr << "Failed to initialize engine. Exiting." << std::endl;
        return -1;
    }

    // ======== Load Demo Chart ========
    auto* chartLoader = engine.GetChartLoader();
    auto* renderer = engine.GetRenderer();

    // Try to load the sample chart
    std::unique_ptr<Ore::Chart> demoChart;
    const char* chartPath = "Charts/demo_4k_easy.json";

    if (chartLoader) {
        demoChart = chartLoader->LoadChart(chartPath);
        if (!demoChart) {
            std::cout << "No demo chart found at '" << chartPath
                      << "'. Running in demo mode with no notes." << std::endl;
        }
    }

    // ======== Judge System ========
    Ore::Judge judge;
    if (demoChart) {
        judge.GetScoreData().totalNotes = static_cast<int>(demoChart->notes.size());
    }

    // ======== Setup Game Loop Callbacks ========
    auto* gameLoop = engine.GetRenderer() ? nullptr : nullptr; // Get via engine

    engine.GetRenderer(); // Reference to keep renderer alive

    // Render callback: draw the demo scene
    double elapsedTime = 0.0;
    engine.GetGameLoop()->SetOnRender([&](double dt) {
        Ore::Renderer* r = engine.GetRenderer();
        if (!r) return;

        // Clear with dark background
        r->ClearScreen(20, 20, 30);

        // Draw title
        int centerX = config.windowWidth / 2;

        // Draw lane guides based on laneCount
        if (demoChart) {
            int laneCount = demoChart->laneCount;
            int laneWidth = 80;
            int startX = centerX - (laneCount * laneWidth) / 2;
            int startY = 500;

            for (int i = 0; i < laneCount; ++i) {
                int x = startX + i * laneWidth;
                // Lane background
                r->DrawRect(x, startY, laneWidth - 4, 200, 40, 40, 60);
                // Lane divider
                r->DrawRect(x + laneWidth - 4, startY, 2, 200, 60, 60, 80);
            }
        } else {
            // No chart loaded - show placeholder
            int startX = centerX - 150;
            r->DrawRect(startX, 500, 300, 200, 40, 40, 60);
        }

        // Draw judgment line
        r->DrawRect(0, 498, config.windowWidth, 4, 255, 255, 255, 100);
    });

    // Update callback
    engine.GetInput()->LoadDefaultBindings();

    // ======== Run ========
    engine.Run();

    // ======== Shutdown ========
    demoChart.reset();
    engine.Shutdown();

    std::cout << "Open Rhythm Engine exited cleanly." << std::endl;
    return 0;
}