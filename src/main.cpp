#include <M5Cardputer.h>
#include "persist.h"
#include "intro.h"
#include "slots.h"
#include "blackjack.h"
#include "dice.h"
#include "videopoker.h"
#include "hilo.h"
#include "roulette.h"
#include "texasholdem.h"

static const uint16_t C_BG   = 0x0000;
static const uint16_t C_GOLD = 0xFEA0;
static const uint16_t C_TXT  = 0xFFFF;
static const uint16_t C_DIM  = 0x4208;
static const uint16_t C_RED  = 0xF800;

static enum { MENU, GAME_SLOTS, GAME_BLACKJACK, GAME_DICE, GAME_VIDEOPOKER, GAME_HILO, GAME_ROULETTE, GAME_POKER } appState = MENU;
static int menuSel = 0;
static bool menuSoundOn = false;

static const int NGAMES = 7;

struct GameEntry {
    const char*  name;    // short name — fits in 79px cell
    const char*  desc;    // very short description
    uint16_t     accent;
    const char*  icon;
};

static const GameEntry GAMES[NGAMES] = {
    { "SLOTS",     "3 reels  x100",   0xFEA0, "SL" },
    { "BLACKJACK", "Beat dealer",     0xF800, "BJ" },
    { "DICE",      "5 dice Yahtzee",  0xFFFF, "DI" },
    { "V.POKER",   "Jacks or Better", 0x07FF, "VP" },
    { "HI / LO",   "Higher or lower", 0x07E0, "HL" },
    { "ROULETTE",  "37 pockets",      0xF81F, "RO" },
    { "HOLD'EM",   "3v1 poker",       0xFD20, "TH" },
};

// 3-column × 3-row grid — all 7 games fit without overflow
// Each cell: 79px wide, 35px tall
// Columns at x=0, 80, 160  |  Rows at y=18, 54, 90
// Bottom of row 2: y=125  |  Footer: y=126-134

static const int GRID_COLS  = 3;
static const int CELL_W     = 79;
static const int CELL_H     = 35;
static const int GRID_Y0    = 18;
static const int CELL_YSTEP = 36;

static void drawMenuCell(int idx, bool sel) {
    if (idx >= NGAMES) return;
    auto& d = M5Cardputer.Display;
    int col = idx % GRID_COLS;
    int row = idx / GRID_COLS;
    int x   = col * 80;
    int y   = GRID_Y0 + row * CELL_YSTEP;
    const GameEntry& g = GAMES[idx];

    uint16_t bg = sel ? (uint16_t)0x1082 : C_BG;
    d.fillRect(x, y, CELL_W, CELL_H, bg);

    // Left accent bar (3px)
    d.fillRect(x, y, 3, CELL_H, g.accent);

    // Border: double-gold if selected, dim otherwise
    if (sel) {
        d.drawRect(x, y, CELL_W, CELL_H, C_GOLD);
        d.drawRect(x+1, y+1, CELL_W-2, CELL_H-2, C_GOLD);
    } else {
        d.drawRect(x, y, CELL_W, CELL_H, C_DIM);
    }

    d.setTextSize(1);

    // Icon (row 1, top)
    d.setTextColor(sel ? C_GOLD : g.accent);
    d.setCursor(x+5, y+4); d.print(g.icon);

    // Game name (row 2, middle)
    d.setTextColor(sel ? C_GOLD : C_TXT);
    d.setCursor(x+5, y+14); d.print(g.name);

    // Description (row 3, bottom)
    d.setTextColor(sel ? (uint16_t)0x39E7 : C_DIM);
    d.setCursor(x+5, y+25); d.print(g.desc);
}

static void drawMenu() {
    auto& d = M5Cardputer.Display;
    d.fillScreen(C_BG);

    // Header
    d.fillRect(0, 0, 240, 17, 0x1800);
    d.setTextSize(1); d.setTextColor(C_GOLD);
    const char* t = "<<  A R C A D E  S T A T I O N  >>";
    d.setCursor((240 - d.textWidth(t)) / 2, 5);
    d.print(t);

    // Grid cells
    for (int i = 0; i < NGAMES; i++) drawMenuCell(i, i == menuSel);

    // Footer — session arc (y=126-134)
    d.fillRect(0, 126, 240, 9, 0x0821);
    d.setTextSize(1);
    if (Persist::credits < 30) {
        char footer[56];
        snprintf(footer, sizeof(footer), "LOW CREDITS!  Best:%d  SPC=play", Persist::personalBest);
        d.setTextColor(C_RED);
        d.setCursor((240 - d.textWidth(footer)) / 2, 128);
        d.print(footer);
    } else {
        int goal = min(Persist::sessionStart * 3, 2000);
        char footer[56];
        snprintf(footer, sizeof(footer), "Cr:%d  Goal:%d  Best:%d  SPC=play",
                 Persist::credits, goal, Persist::personalBest);
        d.setTextColor(0x39E7);
        d.setCursor((240 - d.textWidth(footer)) / 2, 128);
        d.print(footer);
    }
}

void setup() {
    auto cfg = M5.config();
    cfg.fallback_board = m5::board_t::board_M5CardputerADV;
    M5Cardputer.begin(cfg, true);
    SPI.begin(40, 39, 14, 12);
    M5Cardputer.Display.setRotation(1);
    randomSeed(analogRead(0) ^ millis());
    Persist::load();
    Persist::newDay();
    Intro::show();
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
                if (c == 'm' || c == 'M') menuSoundOn = !menuSoundOn;

                // D/+ = next game (right/wrap)
                if (c == '+' || c == '=' || c == 'd' || c == 'D') {
                    int prev = menuSel;
                    menuSel = (menuSel + 1) % NGAMES;
                    drawMenuCell(prev, false); drawMenuCell(menuSel, true);
                }
                // A/- = prev game (left/wrap)
                if (c == '-' || c == 'a' || c == 'A') {
                    int prev = menuSel;
                    menuSel = (menuSel - 1 + NGAMES) % NGAMES;
                    drawMenuCell(prev, false); drawMenuCell(menuSel, true);
                }
                // S = skip one row down (3 columns wide)
                if (c == 's' || c == 'S') {
                    int prev = menuSel;
                    menuSel = (menuSel + GRID_COLS) % NGAMES;
                    drawMenuCell(prev, false); drawMenuCell(menuSel, true);
                }
                // W = skip one row up
                if (c == 'w' || c == 'W') {
                    int prev = menuSel;
                    menuSel = (menuSel - GRID_COLS + NGAMES) % NGAMES;
                    drawMenuCell(prev, false); drawMenuCell(menuSel, true);
                }
            }
            if (launch) {
                // Entry fanfare: flash cell 3×
                for (int f = 0; f < 3; f++) {
                    drawMenuCell(menuSel, true);
                    delay(80);
                    auto& d = M5Cardputer.Display;
                    int fc = menuSel % GRID_COLS;
                    int fr = menuSel / GRID_COLS;
                    int cx = fc * 80;
                    int cy = GRID_Y0 + fr * CELL_YSTEP;
                    d.drawRect(cx, cy, CELL_W, CELL_H, C_GOLD);
                    d.drawRect(cx+1, cy+1, CELL_W-2, CELL_H-2, C_GOLD);
                    delay(80);
                    drawMenuCell(menuSel, false);
                    delay(80);
                }
                drawMenuCell(menuSel, true);
                if (menuSoundOn) {
                    M5Cardputer.Speaker.tone(880, 40); delay(50);
                    M5Cardputer.Speaker.tone(1100, 60);
                }
                Persist::sessionStart = Persist::credits;
                Persist::sessionPeak  = Persist::credits;
                switch (menuSel) {
                    case 0: Slots::init();       appState = GAME_SLOTS;      break;
                    case 1: Blackjack::init();   appState = GAME_BLACKJACK;  break;
                    case 2: Dice::init();        appState = GAME_DICE;       break;
                    case 3: VideoPoker::init();  appState = GAME_VIDEOPOKER; break;
                    case 4: HiLo::init();        appState = GAME_HILO;       break;
                    case 5: Roulette::init();    appState = GAME_ROULETTE;   break;
                    case 6: TexasHoldEm::init(); appState = GAME_POKER;      break;
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
        case GAME_POKER:      running = TexasHoldEm::tick(); break;
        default: break;
    }

    if (!running) {
        auto& d = M5Cardputer.Display;
        d.fillRect(0, 58, 240, 18, 0x0821);
        d.setTextSize(1); d.setTextColor(C_GOLD);
        char story[56];
        snprintf(story, sizeof(story), "Session peak: %d  |  Out with: %d",
                 Persist::sessionPeak, Persist::credits);
        d.setCursor((240 - d.textWidth(story)) / 2, 63);
        d.print(story);
        delay(1500);
        Persist::save();
        appState = MENU;
        drawMenu();
    }
}
