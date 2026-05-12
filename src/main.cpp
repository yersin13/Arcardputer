#include <M5Cardputer.h>
#include "img_cherry.h"
#include "img_lemon.h"
#include "img_orange.h"
#include "img_grape.h"
#include "img_watermelon.h"
#include "img_seven.h"
#include "img_bell.h"

// ── Symbols ────────────────────────────────────────────────────────────────
static const uint16_t* const SPRITES[7] = {
    img_seven, img_bell, img_cherry, img_lemon,
    img_orange, img_grape, img_watermelon
};
static const char* const  SYM[7]  = { "7","BEL","CHR","LMN","ORG","GRP","WTR" };
static const uint16_t     SCOL[7] = { 0xF800,0xFFE0,0xF81F,0xFFE0,0xFD20,0xC01F,0x07E0 };
static const int          PAY[7]  = { 100, 30, 15, 8, 5, 3, 2 };
static const int          NSYM    = 7;

// ── Layout ─────────────────────────────────────────────────────────────────
static const int REEL_X[3] = { 18, 87, 156 };
static const int RY = 29, RW = 65, RH = 62;
static const int IMG_SIZE = 48;
static const int IMG_OX   = (RW - IMG_SIZE) / 2;   // 8
static const int IMG_OY   = (RH - IMG_SIZE) / 2;   // 7

// ── Colors ─────────────────────────────────────────────────────────────────
static const uint16_t C_BG   = 0x0000;
static const uint16_t C_TXT  = 0xFFFF;
static const uint16_t C_DIM  = 0x4208;
static const uint16_t C_GOLD = 0xFEA0;

// ── State ──────────────────────────────────────────────────────────────────
static int  credits, bet;
static int  result[3];
static char winMsg[48];
static uint16_t winCol;
static enum { IDLE, SPINNING, FLASHING, GAMEOVER } gState;
static int  tick, stopAt[3], flashTick, lastWin;
static int  spinSym[3];   // currently displayed symbol per reel

// ── Draw helpers ───────────────────────────────────────────────────────────
static void drawReel(int i, int sym, bool spinning, bool hl = false) {
    auto& d = M5Cardputer.Display;
    int x = REEL_X[i];
    uint16_t border = hl ? C_GOLD : (spinning ? C_TXT : C_DIM);
    uint16_t bg     = hl ? (uint16_t)0x0820 : C_BG;

    d.fillRect(x + 1, RY + 1, RW - 2, RH - 2, bg);
    d.drawRect(x, RY, RW, RH, border);
    if (spinning) d.drawRect(x + 1, RY + 1, RW - 2, RH - 2, border);

    d.pushImage(x + IMG_OX, RY + IMG_OY, IMG_SIZE, IMG_SIZE, SPRITES[sym]);
}

static void drawHeader() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0, 0, 240, 16, 0x1800);
    d.setTextSize(1);
    d.setTextColor(C_GOLD);
    const char* t = "<<  S L O T  M A C H I N E  >>";
    d.setCursor((240 - d.textWidth(t)) / 2, 4);
    d.print(t);
}

static void drawHUD() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0, 17, 240, 11, C_BG);
    d.setTextSize(1);
    char left[16], right[20];
    snprintf(left,  sizeof(left),  "BET: %d", bet);
    snprintf(right, sizeof(right), "CREDITS: %d", credits);
    d.setTextColor(C_TXT);
    d.setCursor(4, 18);
    d.print(left);
    d.setCursor(240 - d.textWidth(right) - 4, 18);
    d.print(right);
}

static void drawMsg() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0, 93, 240, 14, C_BG);
    if (!winMsg[0]) return;
    d.setTextSize(1);
    d.setTextColor(winCol);
    d.setCursor((240 - d.textWidth(winMsg)) / 2, 95);
    d.print(winMsg);
}

static void drawFooter() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0, 109, 240, 26, 0x0821);
    d.setTextSize(1);
    // mini pay table
    d.setCursor(2, 111);
    for (int i = 0; i < NSYM; i++) {
        d.setTextColor(SCOL[i]);
        d.print(SYM[i]);
        d.setTextColor(C_DIM);
        char buf[8]; snprintf(buf, sizeof(buf), "=%d ", PAY[i]);
        d.print(buf);
    }
    d.setTextColor(0x39E7);
    d.setCursor(2, 123);
    d.print("SPC/Enter=Spin  +/-=Bet  Q=Reset");
}

static void evalResult() {
    int r0 = result[0], r1 = result[1], r2 = result[2];
    lastWin = 0;
    if (r0 == r1 && r1 == r2) {
        lastWin = bet * PAY[r0];
        if (r0 == 0)
            snprintf(winMsg, sizeof(winMsg), "*** JACKPOT! +%d COINS! ***", lastWin);
        else
            snprintf(winMsg, sizeof(winMsg), "3x %s!  +%d coins", SYM[r0], lastWin);
        winCol = SCOL[r0];
    } else if (r0 == r1 || r1 == r2 || r0 == r2) {
        lastWin = bet;
        snprintf(winMsg, sizeof(winMsg), "Pair!  +%d coin%s", lastWin, lastWin > 1 ? "s" : "");
        winCol = C_TXT;
    } else {
        snprintf(winMsg, sizeof(winMsg), "No win. Try again!");
        winCol = C_DIM;
    }
    credits += lastWin;
}

static void startSpin() {
    if (credits < bet) {
        snprintf(winMsg, sizeof(winMsg), "Not enough credits!");
        winCol = 0xF800;
        drawMsg();
        return;
    }
    credits -= bet;
    result[0] = random(NSYM); result[1] = random(NSYM); result[2] = random(NSYM);
    stopAt[0] = 18 + random(8);
    stopAt[1] = stopAt[0] + 18 + random(8);
    stopAt[2] = stopAt[1] + 18 + random(8);
    tick = 0;
    winMsg[0] = '\0';
    gState = SPINNING;
    drawHUD();
    drawMsg();
}

static void resetGame() {
    credits = 100; bet = 1;
    result[0] = result[1] = result[2] = 0;
    spinSym[0] = spinSym[1] = spinSym[2] = 0;
    winMsg[0] = '\0'; winCol = C_DIM;
    gState = IDLE;
    M5Cardputer.Display.fillScreen(C_BG);
    drawHeader(); drawHUD();
    for (int i = 0; i < 3; i++) drawReel(i, 0, false);
    snprintf(winMsg, sizeof(winMsg), "Press SPACE to spin!");
    winCol = C_DIM;
    drawMsg();
    drawFooter();
}

// ── Arduino entry points ───────────────────────────────────────────────────
void setup() {
    auto cfg = M5.config();
    cfg.fallback_board = m5::board_t::board_M5CardputerADV;
    M5Cardputer.begin(cfg, true);
    SPI.begin(40, 39, 14, 12);
    M5Cardputer.Display.setRotation(1);
    randomSeed(analogRead(0) ^ millis());
    resetGame();
}

void loop() {
    M5Cardputer.update();

    // ── Input ─────────────────────────────────────────────────────────────
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        auto st = M5Cardputer.Keyboard.keysState();
        bool wantSpin = st.enter;
        for (char c : st.word) {
            if (c == ' ') wantSpin = true;
            if (c == 'q' || c == 'Q') { resetGame(); return; }
            if (gState == IDLE || gState == GAMEOVER) {
                if (c == '+' || c == '=') { bet = min(bet+1, 10); drawHUD(); }
                if (c == '-')             { bet = max(bet-1, 1);  drawHUD(); }
            }
        }
        if (wantSpin && gState == IDLE)     startSpin();
        if (wantSpin && gState == GAMEOVER) resetGame();
    }

    // ── Spinning ──────────────────────────────────────────────────────────
    if (gState == SPINNING) {
        tick++;
        for (int i = 0; i < 3; i++) {
            bool stopped = (tick >= stopAt[i]);
            int  sym     = stopped ? result[i] : (tick * (i+2)) % NSYM;
            if (sym != spinSym[i] || stopped) {
                spinSym[i] = sym;
                drawReel(i, sym, !stopped);
            }
        }
        if (tick >= stopAt[2]) {
            evalResult();
            drawHUD();
            drawMsg();
            if (credits <= 0) {
                snprintf(winMsg, sizeof(winMsg), "BROKE! Press SPACE to reset");
                winCol = 0xF800;
                drawMsg();
                gState = GAMEOVER;
            } else if (lastWin > 0) {
                flashTick = 0;
                gState = FLASHING;
            } else {
                gState = IDLE;
            }
        }
    }

    // ── Win flash ────────────────────────────────────────────────────────
    if (gState == FLASHING) {
        flashTick++;
        bool hl = (flashTick / 5) % 2 == 0;
        bool jackpot = (result[0] == result[1] && result[1] == result[2]);
        for (int i = 0; i < 3; i++) {
            bool match = jackpot ||
                (i == 0 && result[0] == result[1]) ||
                (i == 1 && (result[0]==result[1] || result[1]==result[2])) ||
                (i == 2 && result[1] == result[2]);
            drawReel(i, result[i], false, match && hl);
        }
        if (flashTick >= 24) {
            drawHeader();
            for (int i = 0; i < 3; i++) drawReel(i, result[i], false, false);
            gState = IDLE;
        }
    }

    delay(50);
}
