#include <M5Cardputer.h>
#include "slots.h"
#include "blackjack.h"
#include "dice.h"
#include "videopoker.h"
#include "hilo.h"
#include "roulette.h"

static const uint16_t C_BG   = 0x0000;
static const uint16_t C_GOLD = 0xFEA0;
static const uint16_t C_TXT  = 0xFFFF;
static const uint16_t C_DIM  = 0x4208;

static enum { MENU, GAME_SLOTS, GAME_BLACKJACK, GAME_DICE, GAME_VIDEOPOKER, GAME_HILO, GAME_ROULETTE } appState = MENU;
static int menuSel = 0;

static const int NGAMES = 6;

struct GameEntry {
    const char*  name;
    const char*  desc;
    uint16_t     accent;   // left bar + selected border color
    const char*  icon;     // 2-char icon drawn in accent color
};

static const GameEntry GAMES[NGAMES] = {
    { "SLOT MACHINE",  "3 reels  7 symbols",       0xFEA0, "SL" },  // gold
    { "BLACKJACK",     "Beat the dealer to 21",     0xF800, "BJ" },  // red
    { "DICE ROLL",     "5 dice  3 rolls  Yahtzee",  0xFFFF, "DI" },  // white
    { "VIDEO POKER",   "5-card draw  Jacks+",       0x07FF, "VP" },  // cyan
    { "HI / LO",       "Guess higher or lower",     0x07E0, "HL" },  // green
    { "ROULETTE",      "European wheel  37 pockets",0xF81F, "RO" },  // magenta
};

// 2-column × 3-row grid
// Each cell: 116px wide, 34px tall, with 4px gap
// Grid starts at x=2, y=20; cols at x=2, x=122; rows spaced 38px

static const int GRID_X[2]  = { 2, 122 };
static const int GRID_Y0    = 20;
static const int CELL_W     = 116;
static const int CELL_H     = 34;
static const int CELL_YSTEP = 38;

static void drawMenuCell(int idx, bool sel) {
    auto& d = M5Cardputer.Display;
    int col = idx % 2;
    int row = idx / 2;
    int x = GRID_X[col];
    int y = GRID_Y0 + row * CELL_YSTEP;
    const GameEntry& g = GAMES[idx];

    // Background
    uint16_t bg = sel ? (uint16_t)0x1082 : C_BG;
    d.fillRect(x, y, CELL_W, CELL_H, bg);

    // Accent bar (left 4px)
    d.fillRect(x, y, 4, CELL_H, g.accent);

    // Border: gold double-rect if selected, dim single if not
    if (sel) {
        d.drawRect(x, y, CELL_W, CELL_H, C_GOLD);
        d.drawRect(x+1, y+1, CELL_W-2, CELL_H-2, C_GOLD);
    } else {
        d.drawRect(x, y, CELL_W, CELL_H, C_DIM);
    }

    // Icon (2-char, accent color)
    d.setTextSize(1);
    d.setTextColor(sel ? C_GOLD : g.accent);
    d.setCursor(x + 8, y + 4);
    d.print(g.icon);

    // Game name
    d.setTextColor(sel ? C_GOLD : C_TXT);
    d.setCursor(x + 26, y + 4);
    d.print(g.name);

    // Description
    d.setTextColor(C_DIM);
    d.setCursor(x + 8, y + 16);
    d.print(g.desc);

    // Arrow indicator if selected
    if (sel) {
        d.setTextColor(C_GOLD);
        d.setCursor(x + CELL_W - 10, y + (CELL_H - 8) / 2);
        d.print(">");
    }
}

static void drawMenu() {
    auto& d = M5Cardputer.Display;
    d.fillScreen(C_BG);

    // Header
    d.fillRect(0, 0, 240, 18, 0x1800);
    d.setTextSize(1); d.setTextColor(C_GOLD);
    const char* t = "<<  A R C A D E  S T A T I O N  >>";
    d.setCursor((240 - d.textWidth(t)) / 2, 5);
    d.print(t);

    // Grid cells
    for (int i = 0; i < NGAMES; i++) {
        drawMenuCell(i, i == menuSel);
    }

    // Footer
    d.fillRect(0, 132, 240, 3, 0x0821);
    d.fillRect(0, 132, 240, 3, C_DIM);
    d.setTextSize(1); d.setTextColor(0x39E7);
    const char* hint = "Arrows / +- : select     SPACE / Enter : play";
    d.setCursor((240 - d.textWidth(hint)) / 2, 128);
    d.print(hint);
}

void setup() {
    auto cfg = M5.config();
    cfg.fallback_board = m5::board_t::board_M5CardputerADV;
    M5Cardputer.begin(cfg, true);
    SPI.begin(40, 39, 14, 12);
    M5Cardputer.Display.setRotation(1);
    randomSeed(analogRead(0) ^ millis());
    drawMenu();
}

void loop() {
    M5Cardputer.update();

    if (appState == MENU) {
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            bool launch = st.enter;
            for (char c : st.word) {
                if (c == ' ') launch = true;
                // Right / Down / + moves forward
                if (c == '+' || c == '=' || c == 'd' || c == 'D') {
                    int prev = menuSel;
                    menuSel = (menuSel + 1) % NGAMES;
                    drawMenuCell(prev, false);
                    drawMenuCell(menuSel, true);
                }
                // Left / Up / - moves back
                if (c == '-' || c == 'a' || c == 'A') {
                    int prev = menuSel;
                    menuSel = (menuSel - 1 + NGAMES) % NGAMES;
                    drawMenuCell(prev, false);
                    drawMenuCell(menuSel, true);
                }
                // Down-column: skip 2 indices
                if (c == 's' || c == 'S') {
                    int prev = menuSel;
                    menuSel = (menuSel + 2) % NGAMES;
                    drawMenuCell(prev, false);
                    drawMenuCell(menuSel, true);
                }
                if (c == 'w' || c == 'W') {
                    int prev = menuSel;
                    menuSel = (menuSel - 2 + NGAMES) % NGAMES;
                    drawMenuCell(prev, false);
                    drawMenuCell(menuSel, true);
                }
            }
            if (launch) {
                switch (menuSel) {
                    case 0: Slots::init();      appState = GAME_SLOTS;      break;
                    case 1: Blackjack::init();  appState = GAME_BLACKJACK;  break;
                    case 2: Dice::init();       appState = GAME_DICE;       break;
                    case 3: VideoPoker::init(); appState = GAME_VIDEOPOKER; break;
                    case 4: HiLo::init();       appState = GAME_HILO;       break;
                    case 5: Roulette::init();   appState = GAME_ROULETTE;   break;
                }
            }
        }
        return;
    }

    bool running = true;
    switch (appState) {
        case GAME_SLOTS:      running = Slots::tick();      break;
        case GAME_BLACKJACK:  running = Blackjack::tick();  break;
        case GAME_DICE:       running = Dice::tick();       break;
        case GAME_VIDEOPOKER: running = VideoPoker::tick(); break;
        case GAME_HILO:       running = HiLo::tick();       break;
        case GAME_ROULETTE:   running = Roulette::tick();   break;
        default: break;
    }

    if (!running) {
        appState = MENU;
        drawMenu();
    }
}
