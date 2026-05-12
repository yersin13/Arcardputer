#include <M5Cardputer.h>
#include "slots.h"
#include "blackjack.h"
#include "dice.h"

static const uint16_t C_BG   = 0x0000;
static const uint16_t C_GOLD = 0xFEA0;
static const uint16_t C_TXT  = 0xFFFF;
static const uint16_t C_DIM  = 0x4208;

static enum { MENU, GAME_SLOTS, GAME_BLACKJACK, GAME_DICE } appState = MENU;
static int menuSel = 0;

static const int    NGAMES   = 3;
static const char*  NAMES[]  = { "SLOT MACHINE", "BLACKJACK", "DICE ROLL" };
static const char*  DESCS[]  = { "3 reels  7 symbols", "Beat the dealer to 21", "5 dice  3 rolls" };

static void drawMenu() {
    auto& d = M5Cardputer.Display;
    d.fillScreen(C_BG);

    d.fillRect(0, 0, 240, 18, 0x1800);
    d.setTextSize(1); d.setTextColor(C_GOLD);
    const char* t = "<<  A R C A D E  S T A T I O N  >>";
    d.setCursor((240 - d.textWidth(t)) / 2, 5);
    d.print(t);

    for (int i = 0; i < NGAMES; i++) {
        int y = 26 + i * 32;
        bool sel = (i == menuSel);
        if (sel) {
            d.fillRect(6, y - 2, 228, 26, 0x1082);
            d.drawRect(6, y - 2, 228, 26, C_GOLD);
        }
        d.setTextColor(sel ? C_GOLD : C_TXT);
        d.setCursor(16, y + 3);
        d.print(sel ? "> " : "  ");
        d.print(NAMES[i]);
        d.setTextColor(C_DIM);
        d.setCursor(16, y + 14);
        d.print("   "); d.print(DESCS[i]);
    }

    d.fillRect(0, 122, 240, 13, 0x0821);
    d.setTextColor(0x39E7);
    d.setCursor(4, 125);
    d.print("+/-=select   SPACE/Enter=play");
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
                if (c == '+' || c == '=') { menuSel = (menuSel + 1) % NGAMES; drawMenu(); }
                if (c == '-')             { menuSel = (menuSel - 1 + NGAMES) % NGAMES; drawMenu(); }
            }
            if (launch) {
                if (menuSel == 0) { Slots::init();     appState = GAME_SLOTS; }
                if (menuSel == 1) { Blackjack::init(); appState = GAME_BLACKJACK; }
                if (menuSel == 2) { Dice::init();      appState = GAME_DICE; }
            }
        }
        return;
    }

    bool running = true;
    if (appState == GAME_SLOTS)     running = Slots::tick();
    if (appState == GAME_BLACKJACK) running = Blackjack::tick();
    if (appState == GAME_DICE)      running = Dice::tick();

    if (!running) {
        appState = MENU;
        drawMenu();
    }
}
