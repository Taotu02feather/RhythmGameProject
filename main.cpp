/**
 * Open Rhythm Engine — Startup (v0.2.0-dev)
 * ==========================================
 *
 * Pages:
 *   1. Main Menu — Play / Settings / Quit
 *   2. Settings — Lane count 2K-8K + key rebinding (default A-S-D-F-G-H-J-K)
 *   3. Play — Loads demo chart, keyboard input visualization
 *
 * Controls:
 *   Menu:   UP/DOWN arrows to select, ENTER to confirm, ESC to quit
 *   Settings: LEFT/RIGHT to change lane count, ENTER on key slot to rebind
 *             Press any A-Z key to assign, BACKSPACE to return
 *   Play:    Press lane keys to play, ESC to return to main menu
 */

#include "Core/Engine.h"
#include "Core/GameLoop.h"
#include "Core/Renderer.h"
#include "Core/Input.h"
#include "Chart/ChartLoader.h"
#include "Chart/ChartTypes.h"
#include "Gameplay/Judge.h"
#include "Resource/ResourceManager.h"

#include <iostream>
#include <memory>
#include <filesystem>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

// ============================================================================
// Page state machine
// ============================================================================
enum class Page { MainMenu, Settings, Play, Quit };

// ============================================================================
// Scancode -> readable key name
// ============================================================================
static const char* ScancodeToName(SDL_Scancode sc) {
    switch (sc) {
        case SDL_SCANCODE_A: return "A"; case SDL_SCANCODE_B: return "B";
        case SDL_SCANCODE_C: return "C"; case SDL_SCANCODE_D: return "D";
        case SDL_SCANCODE_E: return "E"; case SDL_SCANCODE_F: return "F";
        case SDL_SCANCODE_G: return "G"; case SDL_SCANCODE_H: return "H";
        case SDL_SCANCODE_I: return "I"; case SDL_SCANCODE_J: return "J";
        case SDL_SCANCODE_K: return "K"; case SDL_SCANCODE_L: return "L";
        case SDL_SCANCODE_M: return "M"; case SDL_SCANCODE_N: return "N";
        case SDL_SCANCODE_O: return "O"; case SDL_SCANCODE_P: return "P";
        case SDL_SCANCODE_Q: return "Q"; case SDL_SCANCODE_R: return "R";
        case SDL_SCANCODE_S: return "S"; case SDL_SCANCODE_T: return "T";
        case SDL_SCANCODE_U: return "U"; case SDL_SCANCODE_V: return "V";
        case SDL_SCANCODE_W: return "W"; case SDL_SCANCODE_X: return "X";
        case SDL_SCANCODE_Y: return "Y"; case SDL_SCANCODE_Z: return "Z";
        case SDL_SCANCODE_SPACE:  return "Spc";
        case SDL_SCANCODE_RETURN: return "Enter";
        case SDL_SCANCODE_ESCAPE: return "ESC";
        case SDL_SCANCODE_BACKSPACE: return "Bksp";
        case SDL_SCANCODE_UP:    return "Up";
        case SDL_SCANCODE_DOWN:  return "Dn";
        case SDL_SCANCODE_LEFT:  return "Lt";
        case SDL_SCANCODE_RIGHT: return "Rt";
        default: return "?";
    }
}

// ============================================================================
// Key binding cache
// ============================================================================
struct KeyBindingCache {
    SDL_Scancode laneKeys[8];
    int laneCount = 4;

    void SetDefaults() {
        SDL_Scancode defaults[8] = {
            SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_F,
            SDL_SCANCODE_G, SDL_SCANCODE_H, SDL_SCANCODE_J, SDL_SCANCODE_K
        };
        for (int i = 0; i < 8; ++i) laneKeys[i] = defaults[i];
        laneCount = 4;
    }

    int FindLaneByScancode(SDL_Scancode sc) const {
        for (int i = 0; i < laneCount; ++i)
            if (laneKeys[i] == sc) return i;
        return -1;
    }
};

// ============================================================================
// Apply bindings to Input system
// ============================================================================
static void ApplyKeyBindings(Ore::Input* input, const KeyBindingCache& cache) {
    input->LoadDefaultBindings();
    for (int i = 0; i < cache.laneCount; ++i) {
        Ore::GameAction action = static_cast<Ore::GameAction>(
            static_cast<int>(Ore::GameAction::Lane0) + i);
        input->BindKey(cache.laneKeys[i], action);
    }
}

// ============================================================================
// main()
// ============================================================================
int main(int argc, char* argv[]) {
    // Set console to UTF-8 on Windows
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::cout << "========================================" << std::endl;
    std::cout << "  Open Rhythm Engine v0.1.0" << std::endl;
    std::cout << "========================================" << std::endl;

    // --- Auto-detect project root ---
    namespace fs = std::filesystem;
    fs::path exePath = fs::current_path();
    bool foundRoot = false;
    std::string searchRel[] = { ".", "..", "../.." };
    for (const auto& rel : searchRel) {
        fs::path cand = exePath / rel;
        if (fs::exists(cand / "Charts" / "demo_4k_easy.json")) {
            exePath = fs::absolute(cand);
            foundRoot = true;
            break;
        }
    }
    if (!foundRoot) exePath = fs::current_path();
    std::cout << "[Info] Project root: " << exePath.string() << std::endl;
    fs::current_path(exePath);

    // --- Engine init ---
    Ore::EngineConfig config;
    config.windowTitle = "Open Rhythm Engine v0.1.0";
    config.windowWidth = 1280;
    config.windowHeight = 720;

    Ore::Engine engine(config);
    if (!engine.Initialize()) {
        std::cerr << "Failed to initialize engine. Exiting." << std::endl;
        return -1;
    }
    engine.GetResourceManager()->SetAssetRoot(exePath.string());

    auto* renderer = engine.GetRenderer();
    auto* input = engine.GetInput();
    auto* chartLoader = engine.GetChartLoader();

    // --- State ---
    Page currentPage = Page::MainMenu;
    Page nextPage = Page::MainMenu;
    KeyBindingCache keyCache;
    keyCache.SetDefaults();

    int menuSel = 0;
    const int MENU_ITEMS = 3;

    int settingSel = 0;
    const int SETTING_LANE_IDX = 0;
    const int SETTING_KEY_START = 1;
    int SETTING_BACK_IDX = 1;
    bool waitingForKey = false;
    int waitingLane = -1;

    std::unique_ptr<Ore::Chart> demoChart;
    double lanePress[8] = {-10,-10,-10,-10,-10,-10,-10,-10};
    double elapsed = 0.0;
    const double flashDur = 0.15;
    double pageSwTime = 0.0;
    double totalT = 0.0;

    ApplyKeyBindings(input, keyCache);

    // ======================== UPDATE ========================
    engine.GetGameLoop()->SetOnUpdate([&](double dt) {
        totalT += dt;

        // ESC handling
        if (input->IsActionPressed(Ore::GameAction::Pause)) {
            if (totalT - pageSwTime > 0.3) {
                switch (currentPage) {
                    case Page::MainMenu: nextPage = Page::Quit; break;
                    case Page::Settings:
                        nextPage = Page::MainMenu; pageSwTime = totalT; break;
                    case Page::Play:
                        nextPage = Page::MainMenu; pageSwTime = totalT; break;
                    default: break;
                }
            }
        }

        bool confirm = input->IsActionPressed(Ore::GameAction::Confirm) ||
                       input->IsKeyPressed(SDL_SCANCODE_RETURN);
        bool protect = (totalT - pageSwTime) > 0.25;

        switch (currentPage) {

        // --- MAIN MENU ---
        case Page::MainMenu: {
            if (input->IsKeyPressed(SDL_SCANCODE_UP))
                menuSel = (menuSel - 1 + MENU_ITEMS) % MENU_ITEMS;
            if (input->IsKeyPressed(SDL_SCANCODE_DOWN))
                menuSel = (menuSel + 1) % MENU_ITEMS;
            if (confirm && protect) {
                pageSwTime = totalT;
                if (menuSel == 0) {
                    demoChart = chartLoader->LoadChart("Charts/demo_4k_easy.json");
                    if (demoChart) {
                        keyCache.laneCount = demoChart->laneCount;
                        ApplyKeyBindings(input, keyCache);
                    }
                    nextPage = Page::Play;
                } else if (menuSel == 1) {
                    settingSel = 0; waitingForKey = false; nextPage = Page::Settings;
                } else {
                    nextPage = Page::Quit;
                }
            }
            break;
        }

        // --- SETTINGS ---
        case Page::Settings: {
            if (waitingForKey) {
                // Wait for a key press (A-Z only)
                for (int k = SDL_SCANCODE_A; k <= SDL_SCANCODE_Z; ++k) {
                    if (input->IsKeyPressed(static_cast<SDL_Scancode>(k))) {
                        SDL_Scancode ns = static_cast<SDL_Scancode>(k);
                        int occ = keyCache.FindLaneByScancode(ns);
                        if (occ >= 0 && occ != waitingLane)
                            keyCache.laneKeys[occ] = keyCache.laneKeys[waitingLane];
                        keyCache.laneKeys[waitingLane] = ns;
                        ApplyKeyBindings(input, keyCache);
                        waitingForKey = false; waitingLane = -1;
                        std::cout << "[Settings] Lane " << waitingLane
                                  << " -> " << ScancodeToName(ns) << std::endl;
                        break;
                    }
                }
                if (input->IsKeyPressed(SDL_SCANCODE_ESCAPE))
                    { waitingForKey = false; waitingLane = -1; }
                break;
            }

            int maxS = 1 + keyCache.laneCount + 1;
            SETTING_BACK_IDX = maxS - 1;

            if (input->IsKeyPressed(SDL_SCANCODE_UP))
                settingSel = (settingSel - 1 + maxS) % maxS;
            if (input->IsKeyPressed(SDL_SCANCODE_DOWN))
                settingSel = (settingSel + 1) % maxS;

            if (settingSel == SETTING_LANE_IDX) {
                if (input->IsKeyPressed(SDL_SCANCODE_LEFT)) {
                    keyCache.laneCount = std::max(2, keyCache.laneCount - 1);
                    ApplyKeyBindings(input, keyCache);
                }
                if (input->IsKeyPressed(SDL_SCANCODE_RIGHT)) {
                    keyCache.laneCount = std::min(8, keyCache.laneCount + 1);
                    ApplyKeyBindings(input, keyCache);
                }
            }

            if (confirm && protect) {
                if (settingSel == SETTING_BACK_IDX) {
                    nextPage = Page::MainMenu; pageSwTime = totalT;
                } else if (settingSel >= SETTING_KEY_START &&
                           settingSel < SETTING_KEY_START + keyCache.laneCount) {
                    waitingForKey = true;
                    waitingLane = settingSel - SETTING_KEY_START;
                    std::cout << "[Settings] Waiting for key for lane " << waitingLane << "..." << std::endl;
                }
            }

            if (input->IsKeyPressed(SDL_SCANCODE_BACKSPACE) && protect) {
                nextPage = Page::MainMenu; pageSwTime = totalT;
            }
            break;
        }

        // --- PLAY ---
        case Page::Play: {
            elapsed += dt;
            int lc = keyCache.laneCount;
            for (int i = 0; i < lc; ++i) {
                Ore::GameAction a = static_cast<Ore::GameAction>(
                    static_cast<int>(Ore::GameAction::Lane0) + i);
                if (input->IsActionPressed(a)) {
                    lanePress[i] = elapsed;
                    std::cout << "[Play] Lane " << i << " (" << ScancodeToName(keyCache.laneKeys[i]) << ") hit!" << std::endl;
                }
            }
            break;
        }
        default: break;
        }
    });

    // ======================== RENDER ========================
    engine.GetGameLoop()->SetOnRender([&](double dt) {
        Ore::Renderer* r = renderer;
        const int W = config.windowWidth;
        const int H = config.windowHeight;
        const int CX = W / 2;

        if (nextPage != currentPage) {
            currentPage = nextPage;
            if (currentPage == Page::Play) elapsed = 0.0;
            if (currentPage == Page::Quit) engine.Quit();
        }

        const int scale = 2; // pixel font scale

        switch (currentPage) {

        // ==================== MAIN MENU ====================
        case Page::MainMenu: {
            r->ClearScreen(14, 14, 30);

            // Title bar
            r->DrawRect(0, 0, W, 75, 8, 8, 22);
            r->DrawPixelText("OPEN RHYTHM ENGINE", CX - 110, 8, 255, 220, 80, 255, scale + 1);
            r->DrawPixelText("v0.1.0", CX + 95, 28, 180, 180, 200, 255, scale);

            // Menu items
            const char* labels[3] = { "[ Play ]", "[ Settings ]", "[ Quit ]" };
            int colors[3][3] = {{40,140,220},{80,80,180},{200,60,60}};
            int startY = 160;
            for (int i = 0; i < 3; ++i) {
                int y = startY + i * 80;
                // Button background
                int bx = CX - 160;
                if (menuSel == i) {
                    r->DrawRect(bx - 3, y - 3, 326, 56, 255, 200, 60, 200);
                    r->DrawRect(bx, y, 320, 50, colors[i][0] + 40, colors[i][1] + 30, colors[i][2] + 20, 255);
                } else {
                    r->DrawRect(bx, y, 320, 50, colors[i][0], colors[i][1], colors[i][2], 200);
                }
                r->DrawPixelText(labels[i], CX - 40, y + 10, 255, 255, 255, 255, scale + 1);
            }

            // Info tips
            r->DrawPixelText("ARROWS: Select   ENTER: Confirm   ESC: Quit", CX - 180, 560, 180, 180, 200, 255, scale);
            break;
        }

        // ==================== SETTINGS ====================
        case Page::Settings: {
            r->ClearScreen(14, 14, 30);
            r->DrawRect(0, 0, W, 55, 8, 8, 22);
            r->DrawPixelText("SETTINGS", CX - 45, 8, 255, 200, 60, 255, scale + 1);

            int y0 = 80;
            int rowH = 38;
            int maxS = 1 + keyCache.laneCount + 1;
            SETTING_BACK_IDX = maxS - 1;

            // Lane count row
            {
                bool sel = (settingSel == SETTING_LANE_IDX);
                r->DrawRect(60, y0, 340, rowH, sel ? 50 : 25, sel ? 90 : 50, sel ? 150 : 80, 200);
                if (sel) r->DrawRect(58, y0 - 2, 344, rowH + 4, 255, 200, 60, 180);

                char buf[64];
                snprintf(buf, sizeof(buf), "Lane Count: %dK  [<] [>]", keyCache.laneCount);
                r->DrawPixelText(buf, 70, y0 + 8, 255, 255, 255, 255, scale);
            }
            y0 += rowH + 8;

            // Key binding rows
            for (int i = 0; i < keyCache.laneCount; ++i) {
                bool sel = (settingSel == SETTING_KEY_START + i);
                r->DrawRect(60, y0, 340, rowH, sel ? 40 : 20, sel ? 40 : 25, sel ? 70 : 40, 200);
                if (sel) r->DrawRect(58, y0 - 2, 344, rowH + 4, 255, 200, 60, 180);

                char buf[64];
                const char* kn = ScancodeToName(keyCache.laneKeys[i]);
                if (waitingForKey && waitingLane == i)
                    snprintf(buf, sizeof(buf), "Lane %d: [Press any key...]", i);
                else
                    snprintf(buf, sizeof(buf), "Lane %d: %s", i, kn);
                r->DrawPixelText(buf, 70, y0 + 8, 255, 255, 255, 255, scale);
                y0 += rowH + 4;
            }

            // Back button
            {
                bool sel = (settingSel == SETTING_BACK_IDX);
                r->DrawRect(60, y0 + 6, 340, rowH, sel ? 80 : 50, sel ? 40 : 30, sel ? 40 : 25, 200);
                if (sel) r->DrawRect(58, y0 + 4, 344, rowH + 4, 255, 200, 60, 180);
                r->DrawPixelText("[ Back to Menu ]", 130, y0 + 14, 255, 200, 180, 255, scale);
            }

            // Help text (right side)
            r->DrawRect(470, 80, W - 490, 350, 18, 18, 36, 180);
            r->DrawPixelText("CONTROLS:", 490, 95, 255, 220, 100, 255, scale);
            r->DrawPixelText("UP/DOWN: Navigate", 490, 130, 200, 200, 220, 255, scale);
            r->DrawPixelText("LEFT/RIGHT: Change lanes", 490, 160, 200, 200, 220, 255, scale);
            r->DrawPixelText("ENTER: Select / Set key", 490, 190, 200, 200, 220, 255, scale);
            r->DrawPixelText("Press new key (A-Z) to assign", 490, 220, 200, 200, 220, 255, scale);
            r->DrawPixelText("ESC: Cancel key wait", 490, 250, 200, 200, 220, 255, scale);
            r->DrawPixelText("BACKSPACE: Return to menu", 490, 280, 200, 200, 220, 255, scale);
            r->DrawPixelText("Default: A-S-D-F-G-H-J-K", 490, 320, 180, 180, 200, 255, scale);
            break;
        }

        // ==================== PLAY ====================
        case Page::Play: {
            r->ClearScreen(14, 14, 26);

            int lc = keyCache.laneCount;
            int lw = 90;
            int lh = 200;
            int lsY = 420;
            int jlY = lsY - 2;
            int sX = CX - (lc * lw) / 2;

            // Top bar
            r->DrawRect(0, 0, W, 60, 8, 8, 20);
            r->DrawPixelText("PLAY MODE", CX - 50, 8, 255, 200, 60, 255, scale + 1);
            r->DrawPixelText("ESC to return", CX + 80, 30, 160, 160, 180, 255, scale - 1);

            // Lanes
            for (int i = 0; i < lc; ++i) {
                int x = sX + i * lw;
                bool flash = (elapsed - lanePress[i]) < flashDur;
                Ore::GameAction a = static_cast<Ore::GameAction>(
                    static_cast<int>(Ore::GameAction::Lane0) + i);
                bool held = input->IsActionDown(a);

                if (flash || held) {
                    r->DrawRect(x, lsY, lw - 4, lh, 80, 55, 130);
                    r->DrawRect(x, lsY, lw - 4, 5, 150, 110, 255);
                    r->DrawRect(x, lsY + lh - 5, lw - 4, 5, 150, 110, 255);
                } else {
                    r->DrawRect(x, lsY, lw - 4, lh, 30, 30, 48);
                }
                // Separator
                r->DrawRect(x + lw - 4, lsY, 2, lh, 45, 45, 68);

                // Key label
                int ly = lsY + lh + 8;
                int llw = 60, llh = 28;
                int llx = x + (lw - 4 - llw) / 2;
                if (flash || held) {
                    r->DrawRect(llx, ly, llw, llh, 120, 95, 190, 220);
                } else {
                    r->DrawRect(llx, ly, llw, llh, 40, 40, 65, 180);
                }
                const char* kn = ScancodeToName(keyCache.laneKeys[i]);
                r->DrawPixelText(kn, llx + 10 + (strlen(kn) == 1 ? 4 : 0), ly + 4, 255, 255, 255, 255, scale);
            }

            // Judgment line
            r->DrawRect(0, jlY - 1, W, 4, 255, 190, 50, 200);
            r->DrawRect(0, jlY + 3, W, 2, 255, 90, 25, 80);

            // Bottom bar
            r->DrawRect(0, H - 40, W, 40, 6, 6, 16, 200);

            // Chart info
            if (demoChart) {
                r->DrawRect(W - 260, 70, 245, 100, 14, 14, 28, 200);
                char b1[64];
                snprintf(b1, sizeof(b1), "Demo: %d notes", (int)demoChart->notes.size());
                r->DrawPixelText(b1, W - 250, 78, 220, 220, 255, 255, scale);
                snprintf(b1, sizeof(b1), "BPM: 120 | %dK", lc);
                r->DrawPixelText(b1, W - 250, 108, 200, 200, 220, 255, scale);
                r->DrawPixelText("Difficulty: Easy | Lv.1", W - 250, 138, 200, 200, 220, 255, scale);
            }
            break;
        }
        default: break;
        }
    });

    // --- Startup info ---
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Engine started!" << std::endl;
    std::cout << "  Default keys (A-L row):";
    for (int i = 0; i < 8; ++i)
        std::cout << " " << ScancodeToName(keyCache.laneKeys[i]);
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;

    engine.Run();

    demoChart.reset();
    engine.Shutdown();
    std::cout << "Open Rhythm Engine exited cleanly." << std::endl;
    return 0;
                {
                    int x = leftCol;
                    int boxW = 340;
                    bool sel = (settingSelection == SETTING_BACK_IDX);
                    DrawSettingOption(r, x, y + 10, boxW, rowH, 80, 50, 50, sel);
                }

                // 右侧帮助文字区域
                r->DrawRect(rightCol, 100, 550, 300, 22, 22, 40, 150);
                r->DrawRect(rightCol + 10, 110, 80, 30, 60, 60, 100, 200);
                // 帮助: ↑↓ 导航, ←→ 切换轨道数, Enter 修改键位, Backspace 返回
                r->DrawRect(winW - 100, winH - 30, 80, 25, 200, 150, 50, 200);

                break;
            }

            // ====================================================
            // 游玩页面渲染
            // ====================================================
            case Page::Play: {
                r->ClearScreen(18, 18, 28);

                int lc = keyCache.laneCount;
                const int laneWidth = 90;
                const int laneHeight = 220;
                const int laneStartY = 440;
                const int judgeLineY = laneStartY - 2;
                const int startX = centerX - (lc * laneWidth) / 2;

                // 顶部栏
                r->DrawRect(0, 0, winW, 70, 12, 12, 22);

                // 绘制轨道
                for (int i = 0; i < lc; ++i) {
                    int x = startX + i * laneWidth;
                    bool flashed = (elapsedTime - lanePressTime[i]) < pressFlashDuration;
                    Ore::GameAction action = static_cast<Ore::GameAction>(
                        static_cast<int>(Ore::GameAction::Lane0) + i);
                    bool held = input->IsActionDown(action);

                    if (flashed || held) {
                        // 按下高亮
                        r->DrawRect(x, laneStartY, laneWidth - 4, laneHeight, 80, 60, 140);
                        r->DrawRect(x, laneStartY, laneWidth - 4, 6, 150, 120, 255);
                        r->DrawRect(x, laneStartY + laneHeight - 6, laneWidth - 4, 6, 150, 120, 255);
                        r->DrawRect(x + 4, laneStartY + 6, laneWidth - 12, laneHeight - 12,
                                    30, 20, 60, 120);
                    } else {
                        r->DrawRect(x, laneStartY, laneWidth - 4, laneHeight, 35, 35, 55);
                        r->DrawRect(x, laneStartY, laneWidth - 4, 2, 60, 60, 90);
                    }
                    // 分隔线
                    r->DrawRect(x + laneWidth - 4, laneStartY, 2, laneHeight, 50, 50, 75);

                    // 按键标签
                    int labelY = laneStartY + laneHeight + 8;
                    int labelW = 60;
                    int labelH = 30;
                    int labelX = x + (laneWidth - 4 - labelW) / 2;
                    if (flashed || held) {
                        r->DrawRect(labelX, labelY, labelW, labelH, 120, 100, 200, 220);
                    } else {
                        r->DrawRect(labelX, labelY, labelW, labelH, 45, 45, 70, 180);
                    }
                }

                // 判定线
                r->DrawRect(0, judgeLineY - 1, winW, 5, 255, 200, 60, 180);
                r->DrawRect(0, judgeLineY + 4, winW, 2, 255, 100, 30, 80);

                // 底部栏
                r->DrawRect(0, winH - 45, winW, 45, 8, 8, 18, 200);

                // 谱面信息
                if (demoChart) {
                    int infoX = winW - 280;
                    r->DrawRect(infoX, 85, 265, 120, 15, 15, 30, 200);
                }
                break;
            }

            default: break;
        }
    });

    // ======== 输出启动信息 ========
    std::cout << "\n========================================" << std::endl;
    std::cout << "  引擎已启动!" << std::endl;
    std::cout << "  默认键位 (A-L行):";
    for (int i = 0; i < 8; ++i) {
        std::cout << " " << ScancodeToName(keyCache.laneKeys[i]);
    }
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;

    // ======== Run ========
    engine.Run();

    // ======== Shutdown ========
    demoChart.reset();
    engine.Shutdown();
    std::cout << "Open Rhythm Engine exited cleanly." << std::endl;
    return 0;
}