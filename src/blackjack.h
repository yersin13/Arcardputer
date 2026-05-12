#pragma once
#include <M5Cardputer.h>
#include "persist.h"

namespace Blackjack {

static const uint16_t C_BG   = 0x0000;
static const uint16_t C_TXT  = 0xFFFF;
static const uint16_t C_DIM  = 0x4208;
static const uint16_t C_GOLD = 0xFEA0;
static const uint16_t C_RED  = 0xF800;
static const uint16_t C_GRN  = 0x07E0;
static const uint16_t C_CARD = 0xFFFF;  // card face (white)
static const uint16_t C_BACK = 0x000F;  // card back (dark blue)

// ── Card geometry ─────────────────────────────────────────────────────────
// Max 8 cards visible per hand; card 26x36, 4px gap → 8*26+7*4 = 236px
static const int CW = 26;  // card width
static const int CH = 36;  // card height
static const int CG = 4;   // gap between cards
static const int CX0 = 2;  // start x

static const int DEALER_Y = 33;  // top of dealer cards
static const int PLAYER_Y = 81;  // top of player cards

// ── Suit geometry helpers ─────────────────────────────────────────────────
// suit: 0=Spades 1=Hearts 2=Diamonds 3=Clubs
static const uint16_t SUIT_COL[4] = { 0x2104, C_RED, C_RED, 0x2104 }; // dark-gray, red, red, dark-gray

static void drawSuit(int cx, int cy, int suit, uint16_t col) {
    auto& d = M5Cardputer.Display;
    switch (suit) {
        case 0: // Spades: upside-down heart + stem
            d.fillCircle(cx-3, cy-1, 3, col);
            d.fillCircle(cx+3, cy-1, 3, col);
            d.fillTriangle(cx-5,cy+1, cx+5,cy+1, cx,cy+6, col);
            d.fillRect(cx-1, cy+5, 3, 3, col);
            break;
        case 1: // Hearts
            d.fillCircle(cx-3, cy-1, 3, col);
            d.fillCircle(cx+3, cy-1, 3, col);
            d.fillTriangle(cx-5,cy+1, cx+5,cy+1, cx,cy+7, col);
            break;
        case 2: // Diamonds
            d.fillTriangle(cx,cy-5, cx+5,cy, cx-5,cy, col);
            d.fillTriangle(cx-5,cy, cx+5,cy, cx,cy+5, col);
            break;
        case 3: // Clubs: 3 circles + stem
            d.fillCircle(cx,   cy-3, 3, col);
            d.fillCircle(cx-4, cy+1, 3, col);
            d.fillCircle(cx+4, cy+1, 3, col);
            d.fillRect(cx-1, cy+3, 3, 4, col);
            break;
    }
}

// ── Card drawing ──────────────────────────────────────────────────────────
struct Card { int val; int suit; };

static void drawCard(int x, int y, Card c, bool hidden=false) {
    auto& d = M5Cardputer.Display;
    if (hidden) {
        // Card back: dark blue with subtle pattern
        d.fillRoundRect(x, y, CW, CH, 3, C_BACK);
        d.drawRoundRect(x, y, CW, CH, 3, C_DIM);
        for (int py=y+4; py<y+CH-3; py+=4)
            d.drawFastHLine(x+3, py, CW-6, 0x0018);
        return;
    }
    uint16_t sc = SUIT_COL[c.suit];
    d.fillRoundRect(x, y, CW, CH, 3, C_CARD);
    d.drawRoundRect(x, y, CW, CH, 3, sc);

    // Value text top-left
    d.setTextSize(1); d.setTextColor(sc);
    char val[4];
    if      (c.val==1)  strcpy(val,"A");
    else if (c.val==11) strcpy(val,"J");
    else if (c.val==12) strcpy(val,"Q");
    else if (c.val==13) strcpy(val,"K");
    else snprintf(val, sizeof(val), "%d", c.val);
    d.setCursor(x+2, y+2); d.print(val);

    // Suit symbol centered in lower 2/3 of card
    drawSuit(x + CW/2, y + CH/2 + 4, c.suit, sc);
}

// ── State ─────────────────────────────────────────────────────────────────
static Card  deck[52];
static int   deckTop;
static Card  pHand[11], dHand[11];
static int   pCount, dCount;
static int   bet, origBet;
static char  msg[48];
static uint16_t msgCol;
static bool  soundOn  = false;
static int   soundVol = 100;
static enum { BETTING, PLAYER_TURN, DEALER_TURN, RESULT } bjState;

// ── Sound ─────────────────────────────────────────────────────────────────
static void sndCard() { if (!soundOn) return; M5Cardputer.Speaker.tone(1000,20); }
static void sndWin()  { if (!soundOn) return; M5Cardputer.Speaker.tone(880,80); delay(90); M5Cardputer.Speaker.tone(1100,120); }
static void sndLose() { if (!soundOn) return; M5Cardputer.Speaker.tone(300,200); }

// ── Deck ──────────────────────────────────────────────────────────────────
static void shuffleDeck() {
    int k=0;
    for (int s=0;s<4;s++) for (int v=1;v<=13;v++) deck[k++]={v,s};
    for (int i=51;i>0;i--){ int j=random(i+1); Card t=deck[i]; deck[i]=deck[j]; deck[j]=t; }
    deckTop=0;
}

static Card dealCard() {
    if (deckTop>=52) shuffleDeck();
    return deck[deckTop++];
}

static int handTotal(Card* h, int n) {
    int tot=0, aces=0;
    for (int i=0;i<n;i++){
        int v=h[i].val;
        if (v==1){ aces++; tot+=11; }
        else tot+=min(v,10);
    }
    while (tot>21&&aces>0){ tot-=10; aces--; }
    return tot;
}

// ── Draw helpers ──────────────────────────────────────────────────────────
static void drawHeader() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0,0,240,16,0x1800); d.setTextSize(1); d.setTextColor(C_GOLD);
    const char* t = "<<  B L A C K J A C K  >>";
    d.setCursor((240-d.textWidth(t))/2,4); d.print(t);
}

static void drawHUD() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0,17,240,11,C_BG); d.setTextSize(1);
    char l[20], r[24];
    snprintf(l,sizeof(l),"BET: %d",bet);
    snprintf(r,sizeof(r),"CREDITS: %d",Persist::credits);
    d.setTextColor(C_TXT);
    d.setCursor(4,18); d.print(l);
    d.setCursor(240-d.textWidth(r)-4,18); d.print(r);
}

static void drawHandRow(Card* hand, int count, int y, bool hideFirst) {
    auto& d = M5Cardputer.Display;
    d.fillRect(0, y, 240, CH, C_BG);
    for (int i=0; i<count && i<8; i++) {
        drawCard(CX0 + i*(CW+CG), y, hand[i], hideFirst && i==0);
    }
    // Total badge to the right of cards
    if (!hideFirst) {
        int tot = handTotal(hand, count);
        d.setTextSize(1);
        d.setTextColor(tot>21 ? C_RED : C_TXT);
        char b[8]; snprintf(b,sizeof(b),"=%d",tot);
        int bx = CX0 + count*(CW+CG) + 2;
        if (bx+d.textWidth(b)<240){
            d.setCursor(bx, y + CH/2 - 4);
            d.print(b);
        }
    }
}

static void drawLabels() {
    auto& d = M5Cardputer.Display;
    d.setTextSize(1);
    // Dealer label
    d.fillRect(0, DEALER_Y-10, 60, 10, C_BG);
    d.setTextColor(C_DIM); d.setCursor(4, DEALER_Y-9); d.print("DEALER");
    // Player label
    d.fillRect(0, PLAYER_Y-10, 60, 10, C_BG);
    d.setTextColor(C_DIM); d.setCursor(4, PLAYER_Y-9); d.print("YOU");
}

static void drawMsg() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0,118,240,9,C_BG);
    if (!msg[0]) return;
    d.setTextSize(1); d.setTextColor(msgCol);
    d.setCursor((240-d.textWidth(msg))/2,118); d.print(msg);
}

static void drawFooter() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0,127,240,8,0x0821);
    d.setTextSize(1); d.setTextColor(0x39E7); d.setCursor(2,127);
    if      (bjState==BETTING)     d.print("+/-=BET  SPC=DEAL  Q=MENU");
    else if (bjState==PLAYER_TURN) d.print("H=HIT  S=STAND  D=DBL  Q=MENU");
    else                           d.print("SPC/Enter=NEXT  Q=MENU");
}

// ── Animations ────────────────────────────────────────────────────────────

// Flip a single card back→face at its position
static void flipCard(int x, int y, Card c) {
    drawCard(x, y, c, true);   delay(90);
    drawCard(x, y, c, false);
}

// Deal one card with flip animation then update the total badge
static void dealAnimated(Card* hand, int count, int y, bool otherHidden) {
    int x = CX0 + (count-1) * (CW+CG);
    drawCard(x, y, hand[count-1], true);   // back
    delay(100);
    drawCard(x, y, hand[count-1], false);  // face
    // refresh total badge
    if (!otherHidden) {
        auto& d = M5Cardputer.Display;
        int tot = handTotal(hand, count);
        d.setTextSize(1);
        d.setTextColor(tot>21 ? C_RED : C_TXT);
        char b[8]; snprintf(b,sizeof(b),"=%d ",tot);
        int bx = CX0 + count*(CW+CG) + 2;
        d.fillRect(bx, y, 30, CH, C_BG);
        if (bx+d.textWidth(b)<240){ d.setCursor(bx, y+CH/2-4); d.print(b); }
    }
}

// Flash the winning hand border gold (3 pulses) before result message
static void flashWinner(int y, bool win) {
    if (!win) return;
    for (int f=0; f<3; f++) {
        for (int i=0; i<pCount && i<8; i++) {
            auto& d = M5Cardputer.Display;
            d.drawRoundRect(CX0+i*(CW+CG), y, CW, CH, 3, f%2==0 ? C_GOLD : SUIT_COL[pHand[i].suit]);
        }
        delay(120);
    }
}

// ── Game logic ────────────────────────────────────────────────────────────
static void evalAndShow() {
    int p  = handTotal(pHand, pCount);
    int dv = handTotal(dHand, dCount);
    bool pBJ = (pCount==2 && p==21);
    bool dBJ = (dCount==2 && dv==21);

    if (p>21) {
        snprintf(msg,sizeof(msg),"Bust! You lose."); msgCol=C_DIM; sndLose();
    } else if (pBJ && !dBJ) {
        int win = bet + bet/2;
        Persist::credits += bet + win;
        Persist::updateCredits();
        snprintf(msg,sizeof(msg),"Blackjack! +%d",win); msgCol=C_GOLD; sndWin();
        Persist::unlock(Persist::ACH_BLACKJACK);
    } else if (dBJ && !pBJ) {
        snprintf(msg,sizeof(msg),"Dealer Blackjack. You lose."); msgCol=C_DIM; sndLose();
    } else if (dv>21) {
        Persist::credits += bet*2;
        Persist::updateCredits();
        snprintf(msg,sizeof(msg),"Dealer busts! You win +%d",bet*2); msgCol=C_GRN; sndWin();
    } else if (p>dv) {
        Persist::credits += bet*2;
        Persist::updateCredits();
        snprintf(msg,sizeof(msg),"You win! %d vs %d  +%d",p,dv,bet*2); msgCol=C_GRN; sndWin();
    } else if (p==dv) {
        Persist::credits += bet;
        Persist::updateCredits();
        snprintf(msg,sizeof(msg),"Push — %d vs %d. Bet returned.",p,dv); msgCol=C_TXT;
    } else {
        snprintf(msg,sizeof(msg),"Dealer wins. %d vs %d",dv,p); msgCol=C_DIM; sndLose();
    }
    // Brief pause then flash winner before showing message
    delay(300);
    bool playerWon = (msgCol==C_GRN || msgCol==C_GOLD);
    flashWinner(PLAYER_Y, playerWon);
    bjState=RESULT;
    drawHUD(); drawMsg(); drawFooter();
    if (Persist::credits<=0){
        snprintf(msg,sizeof(msg),"BROKE!  SPC=reset  Q=menu"); msgCol=C_RED; drawMsg();
        Persist::credits=100; Persist::save();
    }
}

static void dealerPlay() {
    // Flip dealer's hole card
    delay(300);
    flipCard(CX0, DEALER_Y, dHand[0]);
    // Refresh full row to show total
    drawHandRow(dHand, dCount, DEALER_Y, false);
    delay(400);
    // Dealer draws until 17+
    while (handTotal(dHand,dCount)<17 && dCount<11) {
        dHand[dCount++] = dealCard(); sndCard();
        dealAnimated(dHand, dCount, DEALER_Y, false);
        delay(350);
    }
    evalAndShow();
}

static void deal() {
    if (Persist::credits<bet){ snprintf(msg,sizeof(msg),"Not enough credits!"); msgCol=C_RED; drawMsg(); return; }
    Persist::credits -= bet; origBet = bet;
    Persist::updateCredits();
    pCount=0; dCount=0;
    msg[0]='\0'; bjState=PLAYER_TURN;
    drawHUD();
    // Clear card areas
    M5Cardputer.Display.fillRect(0, DEALER_Y, 240, CH, C_BG);
    M5Cardputer.Display.fillRect(0, PLAYER_Y, 240, CH, C_BG);
    // Deal cards one at a time with flip animation: P D P D
    pHand[pCount++]=dealCard(); sndCard(); dealAnimated(pHand, pCount, PLAYER_Y, false); delay(120);
    dHand[dCount++]=dealCard(); sndCard(); drawCard(CX0, DEALER_Y, dHand[0], true); delay(120); // dealer face-down
    pHand[pCount++]=dealCard(); sndCard(); dealAnimated(pHand, pCount, PLAYER_Y, false); delay(120);
    dHand[dCount++]=dealCard(); sndCard(); dealAnimated(dHand, dCount, DEALER_Y, true); delay(120); // dealer 2nd card face-up, keep 1st hidden
    drawMsg(); drawFooter();
    if (handTotal(pHand,pCount)==21) dealerPlay();  // natural BJ
}

// ── Public ────────────────────────────────────────────────────────────────
void init() {
    bet=5; origBet=5; msg[0]='\0'; msgCol=C_DIM; bjState=BETTING;
    shuffleDeck();
    M5Cardputer.Speaker.setVolume(soundVol);
    auto& d = M5Cardputer.Display;
    d.fillScreen(C_BG);
    drawHeader(); drawHUD(); drawLabels();
    // Empty card areas
    d.fillRect(0, DEALER_Y, 240, CH, C_BG);
    d.fillRect(0, PLAYER_Y, 240, CH, C_BG);
    snprintf(msg,sizeof(msg),"Place your bet and deal!"); msgCol=C_DIM;
    drawMsg(); drawFooter();
}

bool tick() {
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        auto st = M5Cardputer.Keyboard.keysState();
        bool act = st.enter;
        for (char c : st.word) {
            if (c=='q'||c=='Q') return false;
            if (c=='m'||c=='M'){ soundOn=!soundOn; drawFooter(); }
            if (c==']'){ soundVol=min(soundVol+50,250); M5Cardputer.Speaker.setVolume(soundVol); drawFooter(); }
            if (c=='['){ soundVol=max(soundVol-50, 50); M5Cardputer.Speaker.setVolume(soundVol); drawFooter(); }
            if (c==' ') act=true;

            if (bjState==BETTING) {
                if (c=='+'||c=='=') { bet=min(bet+1,20); drawHUD(); }
                if (c=='-')         { bet=max(bet-1,1);  drawHUD(); }
            } else if (bjState==PLAYER_TURN) {
                if (c=='h'||c=='H') {
                    if (pCount<11){
                        pHand[pCount++]=dealCard(); sndCard();
                        drawHandRow(pHand,pCount,PLAYER_Y,false);
                        if (handTotal(pHand,pCount)>21) dealerPlay();
                    }
                }
                if (c=='s'||c=='S') dealerPlay();
                if ((c=='d'||c=='D') && pCount==2 && Persist::credits>=bet) {
                    Persist::credits-=bet; bet*=2;
                    Persist::updateCredits();
                    pHand[pCount++]=dealCard(); sndCard();
                    drawHUD(); drawHandRow(pHand,pCount,PLAYER_Y,false);
                    dealerPlay();
                }
            }
        }
        if (act) {
            if (bjState==BETTING) {
                deal();
            } else if (bjState==RESULT) {
                if (Persist::credits<=0){ Persist::credits=100; Persist::save(); bet=min(origBet,5); }
                bet=origBet; bjState=BETTING; msg[0]='\0';
                auto& d = M5Cardputer.Display;
                d.fillScreen(C_BG);
                drawHeader(); drawHUD(); drawLabels();
                d.fillRect(0,DEALER_Y,240,CH,C_BG);
                d.fillRect(0,PLAYER_Y,240,CH,C_BG);
                snprintf(msg,sizeof(msg),"Place your bet and deal!"); msgCol=C_DIM;
                drawMsg(); drawFooter();
            }
        }
    }
    delay(50);
    return true;
}

} // namespace Blackjack
