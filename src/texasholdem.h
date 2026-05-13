#pragma once
#include <M5Cardputer.h>
#include "persist.h"

namespace TexasHoldEm {

// ── Colors ────────────────────────────────────────────────────────────────
static const uint16_t C_BG    = 0x0000;
static const uint16_t C_TXT   = 0xFFFF;
static const uint16_t C_DIM   = 0x4208;
static const uint16_t C_GOLD  = 0xFEA0;
static const uint16_t C_RED   = 0xF800;
static const uint16_t C_GRN   = 0x07E0;
static const uint16_t C_FELT  = 0x0260;
static const uint16_t C_CARD  = 0xFFFF;
static const uint16_t C_BACK  = 0x000F;
static const uint16_t C_WOOD  = 0x8420;
static const uint16_t C_ACTBG = 0x0821;
static const uint16_t C_HDRBG = 0x1800;

// ── Card geometry ─────────────────────────────────────────────────────────
static const int CW = 26;
static const int CH = 34;

// ── Suit colors ───────────────────────────────────────────────────────────
static const uint16_t SUIT_COL[4] = { 0x2104, C_RED, C_RED, 0x2104 };

// ── Characters ────────────────────────────────────────────────────────────
struct Character {
    const char* name;
    const char* style;
    uint16_t skinCol;
    uint16_t hairCol;
    uint16_t clothCol;
    int aggression;
    int tightness;
};

static const Character CHARS[3] = {
    {"THE COWBOY",  "AGGRESSIVE", 0xD6BA, 0x6200, 0xC980, 8, 3},
    {"THE SHARK",   "TIGHT",      0xBDD7, 0x18C3, 0x0010, 5, 8},
    {"THE ROOKIE",  "LOOSE",      0xFEEA, 0xC400, 0x07FF, 4, 2},
};

// ── Enums ─────────────────────────────────────────────────────────────────
enum GameState {
    SELECT_CHAR, DEAL_ANIM, PLAYER_ACT, CPU_ACT,
    SHOW_FLOP, SHOW_TURN, SHOW_RIVER, SHOWDOWN, ROUND_END
};

enum Expression { EXP_NEUTRAL, EXP_HAPPY, EXP_SAD, EXP_THINK, EXP_SHOCK };

// ── State variables ───────────────────────────────────────────────────────
struct Card { int val; int suit; };

static Card deck[52];
static int  deckTop;
static Card playerHole[2], cpuHole[2], community[5];
static int  communityCount;
static int  pot, playerChips, cpuChips;
static int  playerBet, cpuBet;
static int  roundBet;
static bool playerActed;
static int  selectedChar;
static int  charSelIdx;
static bool playerFolded, cpuFolded;
static int  raiseAmount;
static GameState thState;
static Expression cpuExp;
static int  expTimer;
static int  blinkTimer;
static bool eyesBlink;
static char statusMsg[48];
static uint16_t statusCol;
static int  statusTimer;
static bool soundOn  = false;
static int  soundVol = 100;
static int  handNumber;

// ── Sound ─────────────────────────────────────────────────────────────────
static void sndWin()  { if(!soundOn)return; M5Cardputer.Speaker.tone(880,80); delay(90); M5Cardputer.Speaker.tone(1100,120); }
static void sndLose() { if(!soundOn)return; M5Cardputer.Speaker.tone(300,200); }
static void sndCard() { if(!soundOn)return; M5Cardputer.Speaker.tone(1000,20); }
static void sndChip() { if(!soundOn)return; M5Cardputer.Speaker.tone(1400,15); }

// ── Deck ──────────────────────────────────────────────────────────────────
static void shuffleDeck() {
    int k=0;
    for(int s=0;s<4;s++) for(int v=1;v<=13;v++) deck[k++]={v,s};
    for(int i=51;i>0;i--){ int j=random(i+1); Card t=deck[i]; deck[i]=deck[j]; deck[j]=t; }
    deckTop=0;
}

static Card pullCard() {
    if(deckTop>=52) shuffleDeck();
    return deck[deckTop++];
}

// ── Suit drawing ──────────────────────────────────────────────────────────
static void drawSuitTH(int cx, int cy, int suit, uint16_t col) {
    auto& d = M5Cardputer.Display;
    switch(suit) {
        case 0: // Spades
            d.fillCircle(cx-3,cy-1,3,col);
            d.fillCircle(cx+3,cy-1,3,col);
            d.fillTriangle(cx-5,cy+1,cx+5,cy+1,cx,cy+6,col);
            d.fillRect(cx-1,cy+5,3,3,col);
            break;
        case 1: // Hearts
            d.fillCircle(cx-3,cy-1,3,col);
            d.fillCircle(cx+3,cy-1,3,col);
            d.fillTriangle(cx-5,cy+1,cx+5,cy+1,cx,cy+7,col);
            break;
        case 2: // Diamonds
            d.fillTriangle(cx,cy-5,cx+5,cy,cx-5,cy,col);
            d.fillTriangle(cx-5,cy,cx+5,cy,cx,cy+5,col);
            break;
        case 3: // Clubs
            d.fillCircle(cx,cy-3,3,col);
            d.fillCircle(cx-4,cy+1,3,col);
            d.fillCircle(cx+4,cy+1,3,col);
            d.fillRect(cx-1,cy+3,3,4,col);
            break;
    }
}

// ── Card drawing ──────────────────────────────────────────────────────────
static void drawCardTH(int x, int y, Card c, bool hidden=false) {
    auto& d = M5Cardputer.Display;
    if(hidden) {
        d.fillRoundRect(x,y,CW,CH,3,C_BACK);
        d.drawRoundRect(x,y,CW,CH,3,C_DIM);
        for(int py=y+4;py<y+CH-3;py+=4)
            d.drawFastHLine(x+3,py,CW-6,0x0018);
        return;
    }
    uint16_t sc = SUIT_COL[c.suit];
    d.fillRoundRect(x,y,CW,CH,3,C_CARD);
    d.drawRoundRect(x,y,CW,CH,3,sc);
    d.setTextSize(1); d.setTextColor(sc);
    char val[4];
    if     (c.val==1)  strcpy(val,"A");
    else if(c.val==11) strcpy(val,"J");
    else if(c.val==12) strcpy(val,"Q");
    else if(c.val==13) strcpy(val,"K");
    else snprintf(val,sizeof(val),"%d",c.val);
    d.setCursor(x+2,y+2); d.print(val);
    drawSuitTH(x+CW/2, y+CH/2+3, c.suit, sc);
}

// ── Hand evaluation (copy of VideoPoker evalHand, renamed evalHand5) ──────
static int evalHand5(Card h[5]) {
    int cnt[14]={}, suits[4]={};
    for(int i=0;i<5;i++){ cnt[h[i].val]++; suits[h[i].suit]++; }
    bool flush=(suits[0]==5||suits[1]==5||suits[2]==5||suits[3]==5);
    int v[5]; for(int i=0;i<5;i++) v[i]=h[i].val;
    for(int i=0;i<4;i++) for(int j=i+1;j<5;j++) if(v[j]<v[i]){int t=v[i];v[i]=v[j];v[j]=t;}
    bool royal=(v[0]==1&&v[1]==10&&v[2]==11&&v[3]==12&&v[4]==13);
    bool straight=royal;
    if(!straight){straight=true;for(int i=0;i<4;i++)if(v[i+1]!=v[i]+1){straight=false;break;}}
    int mx=0,pairs=0,trips=0,quads=0;
    for(int vv=1;vv<=13;vv++){
        if(cnt[vv]==4)quads++;
        if(cnt[vv]==3)trips++;
        if(cnt[vv]==2)pairs++;
        if(cnt[vv]>mx)mx=cnt[vv];
    }
    if(flush&&royal)    return 9;
    if(flush&&straight) return 8;
    if(quads)           return 7;
    if(trips&&pairs)    return 6;
    if(flush)           return 5;
    if(straight)        return 4;
    if(trips)           return 3;
    if(pairs==2)        return 2;
    if(pairs==1){ for(int vv=1;vv<=13;vv++) if(cnt[vv]==2&&(vv==1||vv>=11)) return 1; }
    return 0;
}

// ── 7-card best hand evaluator ────────────────────────────────────────────
static int eval7(Card h[7]) {
    int best=0;
    for(int s1=0;s1<7;s1++) for(int s2=s1+1;s2<7;s2++) {
        Card h5[5]; int k=0;
        for(int i=0;i<7;i++) if(i!=s1&&i!=s2) h5[k++]=h[i];
        int r=evalHand5(h5);
        if(r>best) best=r;
    }
    return best;
}

// ── Pre-flop 2-card strength (0-9) ───────────────────────────────────────
static int preFlopStr(Card h[2]) {
    int v1=h[0].val, v2=h[1].val;
    if(v1==1) v1=14; if(v2==1) v2=14;
    if(v1<v2){ int t=v1; v1=v2; v2=t; }
    bool suited=(h[0].suit==h[1].suit);
    if(v1==v2) return min(9, 4+v1/3);
    if(v1==14&&v2>=10) return 8;
    if(v1==14&&suited) return 6;
    if(v1>=11&&v2>=10) return 7;
    if(v1==14) return 5;
    if(suited&&v1-v2<=2) return 5;
    if(v1>=10&&v2>=8)  return 4;
    return 2;
}

// ── CPU hand strength ─────────────────────────────────────────────────────
static int getCpuHandStrength() {
    if(communityCount==0) return preFlopStr(cpuHole);
    if(communityCount<3)  return preFlopStr(cpuHole);
    if(communityCount+2==5) {
        Card h5[5]={cpuHole[0],cpuHole[1],community[0],community[1],community[2]};
        return evalHand5(h5);
    }
    Card h7[7];
    h7[0]=cpuHole[0]; h7[1]=cpuHole[1];
    for(int i=0;i<communityCount;i++) h7[2+i]=community[i];
    if(communityCount+2<7) {
        // pad remaining slots with copies to fill 7
        // but eval7 uses all 7, so just eval with available: use evalHand5 best of available
        // Actually build a 5-card eval from hole+community
        Card h5[5]={cpuHole[0],cpuHole[1],community[0],community[1],community[communityCount-1]};
        return evalHand5(h5);
    }
    return eval7(h7);
}

// ── Portrait drawing ──────────────────────────────────────────────────────
static void drawExpressionMouth(M5GFX& d, int cx, int mouthY, Expression exp) {
    switch(exp) {
        case EXP_NEUTRAL:
            d.drawFastHLine(cx-4,mouthY,8,0x2104);
            break;
        case EXP_HAPPY:
            d.drawLine(cx-4,mouthY+2,cx,mouthY-1,0x2104);
            d.drawLine(cx,mouthY-1,cx+4,mouthY+2,0x2104);
            break;
        case EXP_SAD:
            d.drawLine(cx-4,mouthY-1,cx,mouthY+2,0x2104);
            d.drawLine(cx,mouthY+2,cx+4,mouthY-1,0x2104);
            break;
        case EXP_THINK:
            d.drawFastHLine(cx+1,mouthY,5,0x2104);
            d.fillCircle(cx+8,mouthY-1,2,0x2104);
            break;
        case EXP_SHOCK:
            d.fillCircle(cx,mouthY+1,3,0x2104);
            break;
    }
}

static void drawPortraitSmall(int px, int py, int charIdx, Expression exp, bool blink) {
    auto& d = M5Cardputer.Display;
    const Character& ch = CHARS[charIdx];
    int cx = px+25;
    int cy = py+22;

    // Background + border
    d.fillRoundRect(px,py,50,40,4,0x0210);
    d.drawRoundRect(px,py,50,40,4,ch.clothCol);

    if(charIdx==0) { // COWBOY
        // Hat
        d.fillRect(px+4, cy-14, 42, 3, 0x6200);
        d.fillRect(px+11, cy-23, 28, 10, 0x6200);
        d.drawFastHLine(px+11, cy-14, 28, 0x3100);
        // Head
        d.fillCircle(cx, cy, 11, ch.skinCol);
        // Eyes
        if(!blink) {
            d.fillCircle(cx-4,cy-4,2,0x2104);
            d.fillCircle(cx+4,cy-4,2,0x2104);
        } else {
            d.drawFastHLine(cx-6,cy-4,4,0x2104);
            d.drawFastHLine(cx+2,cy-4,4,0x2104);
        }
        // Mustache
        d.fillRect(cx-5,cy+2,10,2,0x4000);
        // Collar
        d.fillRect(cx-8,py+33,16,5,ch.clothCol);
        // Mouth
        drawExpressionMouth(d, cx, cy+5, exp);
    }
    else if(charIdx==1) { // SHARK
        // Head
        d.fillCircle(cx,cy-1,11,ch.skinCol);
        // Hair slick
        d.fillRect(cx-10,cy-12,20,5,ch.hairCol);
        d.fillTriangle(cx-10,cy-12,cx-14,cy-8,cx-10,cy-8,ch.hairCol);
        // Sunglasses
        d.fillRoundRect(cx-9,cy-5,7,4,1,0x2104);
        d.fillRoundRect(cx+2,cy-5,7,4,1,0x2104);
        d.drawFastHLine(cx-2,cy-3,4,0x4208);
        // Suit collar
        d.fillRect(cx-8,py+33,16,6,ch.clothCol);
        // Tie
        d.fillTriangle(cx-3,py+33,cx,py+29,cx+3,py+33,0xFFFF);
        d.drawLine(cx,py+29,cx,py+38,0x4208);
        // Mouth
        drawExpressionMouth(d, cx, cy+4, exp);
    }
    else { // ROOKIE
        // Messy hair spikes
        d.fillTriangle(cx-8,cy-21,cx-5,cy-13,cx-2,cy-21,ch.hairCol);
        d.fillTriangle(cx-2,cy-23,cx+1,cy-15,cx+4,cy-23,ch.hairCol);
        d.fillTriangle(cx+3,cy-20,cx+6,cy-13,cx+9,cy-20,ch.hairCol);
        // Head
        d.fillCircle(cx,cy-1,11,ch.skinCol);
        // Wide eyes
        if(!blink) {
            d.fillCircle(cx-4,cy-5,3,0xFFFF);
            d.fillCircle(cx+4,cy-5,3,0xFFFF);
            d.fillCircle(cx-4,cy-5,1,0x2104);
            d.fillCircle(cx+4,cy-5,1,0x2104);
        } else {
            d.drawFastHLine(cx-7,cy-5,6,ch.skinCol);
            d.drawFastHLine(cx+1,cy-5,6,ch.skinCol);
        }
        // Hoodie
        d.fillRect(cx-8,py+33,16,6,ch.clothCol);
        // Mouth
        drawExpressionMouth(d, cx, cy+5, exp);
    }
}

static void drawPortraitLarge(int px, int py, int charIdx, Expression exp, bool blink) {
    auto& d = M5Cardputer.Display;
    const Character& ch = CHARS[charIdx];
    int cx = px+40;
    int cy = py+35;

    // Background + border
    d.fillRoundRect(px,py,80,64,6,0x0210);
    d.drawRoundRect(px,py,80,64,6,ch.clothCol);

    if(charIdx==0) { // COWBOY large
        d.fillRect(px+4,cy-22,72,5,0x6200);
        d.fillRect(px+16,cy-38,48,17,0x6200);
        d.drawFastHLine(px+16,cy-22,48,0x3100);
        d.fillCircle(cx,cy,18,ch.skinCol);
        if(!blink) {
            d.fillCircle(cx-6,cy-6,3,0x2104);
            d.fillCircle(cx+6,cy-6,3,0x2104);
        } else {
            d.drawFastHLine(cx-9,cy-6,6,0x2104);
            d.drawFastHLine(cx+3,cy-6,6,0x2104);
        }
        d.fillRect(cx-8,cy+3,16,3,0x4000);
        d.fillRect(cx-13,py+55,26,8,ch.clothCol);
        // Mouth large
        int mY=cy+8;
        switch(exp){
            case EXP_NEUTRAL: d.drawFastHLine(cx-6,mY,12,0x2104); break;
            case EXP_HAPPY:   d.drawLine(cx-6,mY+3,cx,mY-2,0x2104); d.drawLine(cx,mY-2,cx+6,mY+3,0x2104); break;
            case EXP_SAD:     d.drawLine(cx-6,mY-2,cx,mY+3,0x2104); d.drawLine(cx,mY+3,cx+6,mY-2,0x2104); break;
            case EXP_THINK:   d.drawFastHLine(cx+2,mY,8,0x2104); d.fillCircle(cx+13,mY-2,3,0x2104); break;
            case EXP_SHOCK:   d.fillCircle(cx,mY+2,5,0x2104); break;
        }
    }
    else if(charIdx==1) { // SHARK large
        d.fillCircle(cx,cy-1,18,ch.skinCol);
        d.fillRect(cx-16,cy-19,32,8,ch.hairCol);
        d.fillTriangle(cx-16,cy-19,cx-22,cy-13,cx-16,cy-13,ch.hairCol);
        d.fillRoundRect(cx-14,cy-8,11,6,1,0x2104);
        d.fillRoundRect(cx+3,cy-8,11,6,1,0x2104);
        d.drawFastHLine(cx-3,cy-5,6,0x4208);
        d.fillRect(cx-13,py+55,26,9,ch.clothCol);
        d.fillTriangle(cx-5,py+55,cx,py+47,cx+5,py+55,0xFFFF);
        d.drawLine(cx,py+47,cx,py+63,0x4208);
        int mY=cy+7;
        switch(exp){
            case EXP_NEUTRAL: d.drawFastHLine(cx-6,mY,12,0x2104); break;
            case EXP_HAPPY:   d.drawLine(cx-6,mY+3,cx,mY-2,0x2104); d.drawLine(cx,mY-2,cx+6,mY+3,0x2104); break;
            case EXP_SAD:     d.drawLine(cx-6,mY-2,cx,mY+3,0x2104); d.drawLine(cx,mY+3,cx+6,mY-2,0x2104); break;
            case EXP_THINK:   d.drawFastHLine(cx+2,mY,8,0x2104); d.fillCircle(cx+13,mY-2,3,0x2104); break;
            case EXP_SHOCK:   d.fillCircle(cx,mY+2,5,0x2104); break;
        }
    }
    else { // ROOKIE large
        d.fillTriangle(cx-13,cy-34,cx-8,cy-21,cx-3,cy-34,ch.hairCol);
        d.fillTriangle(cx-3,cy-37,cx+2,cy-24,cx+6,cy-37,ch.hairCol);
        d.fillTriangle(cx+5,cy-32,cx+10,cy-21,cx+14,cy-32,ch.hairCol);
        d.fillCircle(cx,cy-1,18,ch.skinCol);
        if(!blink) {
            d.fillCircle(cx-6,cy-8,5,0xFFFF);
            d.fillCircle(cx+6,cy-8,5,0xFFFF);
            d.fillCircle(cx-6,cy-8,2,0x2104);
            d.fillCircle(cx+6,cy-8,2,0x2104);
        } else {
            d.drawFastHLine(cx-11,cy-8,10,ch.skinCol);
            d.drawFastHLine(cx+1,cy-8,10,ch.skinCol);
        }
        d.fillRect(cx-13,py+55,26,9,ch.clothCol);
        int mY=cy+8;
        switch(exp){
            case EXP_NEUTRAL: d.drawFastHLine(cx-6,mY,12,0x2104); break;
            case EXP_HAPPY:   d.drawLine(cx-6,mY+3,cx,mY-2,0x2104); d.drawLine(cx,mY-2,cx+6,mY+3,0x2104); break;
            case EXP_SAD:     d.drawLine(cx-6,mY-2,cx,mY+3,0x2104); d.drawLine(cx,mY+3,cx+6,mY-2,0x2104); break;
            case EXP_THINK:   d.drawFastHLine(cx+2,mY,8,0x2104); d.fillCircle(cx+13,mY-2,3,0x2104); break;
            case EXP_SHOCK:   d.fillCircle(cx,mY+2,5,0x2104); break;
        }
    }
}

// ── Selection screen ──────────────────────────────────────────────────────
static void drawSelectionScreen() {
    auto& d = M5Cardputer.Display;
    d.fillScreen(C_BG);

    // Header
    d.fillRect(0,0,240,16,C_HDRBG);
    d.setTextSize(1); d.setTextColor(C_GOLD);
    const char* hdr="SELECT YOUR OPPONENT";
    d.setCursor((240-d.textWidth(hdr))/2,4); d.print(hdr);

    // Counter top right
    char cnt[8]; snprintf(cnt,sizeof(cnt),"%d/3",charSelIdx+1);
    d.setTextColor(C_DIM);
    d.setCursor(240-d.textWidth(cnt)-3,4); d.print(cnt);

    // Large portrait centered
    drawPortraitLarge(80,18,charSelIdx,EXP_NEUTRAL,false);

    // Character name centered
    const Character& ch=CHARS[charSelIdx];
    d.setTextSize(1); d.setTextColor(ch.clothCol);
    d.setCursor((240-d.textWidth(ch.name))/2,96); d.print(ch.name);

    // Style
    d.setTextColor(C_GOLD);
    d.setCursor((240-d.textWidth(ch.style))/2,108); d.print(ch.style);

    // Flavor text
    const char* flavor[3]={
        "Bluffs hard, bets big",
        "Waits for premium hands",
        "Plays almost anything"
    };
    d.setTextColor(C_DIM);
    d.setCursor((240-d.textWidth(flavor[charSelIdx]))/2,119); d.print(flavor[charSelIdx]);

    // Controls
    const char* ctrl="A/D=browse   SPACE=challenge";
    d.setTextColor(0x39E7);
    d.setCursor((240-d.textWidth(ctrl))/2,127); d.print(ctrl);
}

// ── Draw helpers ──────────────────────────────────────────────────────────
static void drawHeader() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0,0,240,13,C_HDRBG);
    d.drawFastHLine(0,12,240,C_WOOD);
    d.setTextSize(1); d.setTextColor(0xF81F);
    const char* t="TEXAS HOLD'EM";
    d.setCursor(4,3); d.print(t);
    d.setTextColor(C_GOLD);
    char potstr[16]; snprintf(potstr,sizeof(potstr),"POT:%d",pot);
    d.setCursor(240-d.textWidth(potstr)-4,3); d.print(potstr);
}

static void drawOppInfo() {
    auto& d = M5Cardputer.Display;
    // Clear opp info area
    d.fillRect(113,16,124,38,C_FELT);
    d.setTextSize(1);
    d.setTextColor(C_DIM);
    d.setCursor(113,18); d.print(CHARS[selectedChar].name);
    d.setTextColor(C_TXT);
    char chips[20]; snprintf(chips,sizeof(chips),"Chips:%d",cpuChips);
    d.setCursor(113,28); d.print(chips);
    if(cpuBet>0) {
        d.setTextColor(C_GOLD);
        char bet[16]; snprintf(bet,sizeof(bet),"Bet:%d",cpuBet);
        d.setCursor(113,38); d.print(bet);
    }
}

static void drawPlayerInfo() {
    auto& d = M5Cardputer.Display;
    d.fillRect(153,90,84,24,C_FELT);
    d.setTextSize(1);
    d.setTextColor(C_TXT);
    char chips[20]; snprintf(chips,sizeof(chips),"Chips:%d",playerChips);
    d.setCursor(155,92); d.print(chips);
    if(playerBet>0) {
        d.setTextColor(C_GOLD);
        char bet[16]; snprintf(bet,sizeof(bet),"Bet:%d",playerBet);
        d.setCursor(155,103); d.print(bet);
    }
}

static void drawActionBar() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0,123,240,12,C_ACTBG);
    d.drawFastHLine(0,123,240,C_WOOD);
    d.setTextSize(1); d.setTextColor(0x39E7);
    switch(thState) {
        case SELECT_CHAR:
            d.setCursor(2,125); d.print("A/D=browse  SPC=challenge  Q=menu");
            break;
        case PLAYER_ACT: {
            bool facingBet=(roundBet>playerBet);
            if(facingBet) {
                char buf[56];
                snprintf(buf,sizeof(buf),"A=FOLD  S=CALL%d  D=RAISE  +/-=sz",roundBet-playerBet);
                d.setCursor(2,125); d.print(buf);
            } else {
                d.setCursor(2,125); d.print("A=FOLD  S=CHECK  D=BET  Q=menu");
            }
            break;
        }
        case CPU_ACT: {
            char buf[40];
            snprintf(buf,sizeof(buf),"%s is thinking...",CHARS[selectedChar].name);
            d.setCursor(2,125); d.print(buf);
            break;
        }
        case SHOWDOWN:
        case ROUND_END:
            d.setCursor(2,125); d.print("SPC=next hand  Q=menu");
            break;
        default:
            d.setCursor(2,125); d.print("Q=menu");
            break;
    }
}

static void drawStatusArea() {
    auto& d = M5Cardputer.Display;
    // Clear status zone: y=41-52 in felt
    d.fillRect(0,41,240,12,C_FELT);
    if(!statusMsg[0]) return;
    d.setTextSize(1); d.setTextColor(statusCol);
    d.setCursor((240-d.textWidth(statusMsg))/2,43); d.print(statusMsg);
}

static void drawCommunityCards() {
    auto& d = M5Cardputer.Display;
    // Community card positions: x=43,73,103,133,163 y=55
    static const int CX[5]={43,73,103,133,163};
    if(communityCount==0) {
        // Show label
        d.fillRect(0,53,240,38,C_FELT);
        d.setTextSize(1); d.setTextColor(C_DIM);
        const char* lbl="COMMUNITY";
        d.setCursor(2,55); d.print(lbl);
        return;
    }
    // Clear community area
    d.fillRect(0,53,240,38,C_FELT);
    for(int i=0;i<5;i++) {
        if(i<communityCount) {
            drawCardTH(CX[i],55,community[i],false);
        } else {
            // placeholder (empty)
            d.drawRoundRect(CX[i],55,CW,CH,3,C_DIM);
        }
    }
}

static void drawOppCards() {
    // Two face-down cards at x=56,y=16 and x=84,y=16
    drawCardTH(56,16,cpuHole[0],true);
    drawCardTH(84,16,cpuHole[1],true);
}

static void drawPlayerCards() {
    drawCardTH(90,89,playerHole[0],false);
    drawCardTH(120,89,playerHole[1],false);
}

static void redrawPortrait() {
    auto& d = M5Cardputer.Display;
    // Clear portrait area in felt
    d.fillRect(2,14,50,40,C_FELT);
    drawPortraitSmall(2,14,selectedChar,cpuExp,eyesBlink);
}

static void drawHUD() {
    drawHeader();
    drawOppInfo();
    drawPlayerInfo();
}

static void redrawTable() {
    auto& d = M5Cardputer.Display;
    // Felt area only (y=13-122)
    d.fillRect(0,13,240,110,C_FELT);
    redrawPortrait();
    drawOppCards();
    drawOppInfo();
    drawCommunityCards();
    drawPlayerCards();
    drawPlayerInfo();
}

static void redrawAll() {
    auto& d = M5Cardputer.Display;
    d.fillScreen(C_BG);
    drawHeader();
    // Felt
    d.fillRect(0,13,240,110,C_FELT);
    redrawPortrait();
    drawOppCards();
    drawOppInfo();
    drawCommunityCards();
    drawPlayerCards();
    drawPlayerInfo();
    drawActionBar();
    drawStatusArea();
}

// ── Forward declarations ──────────────────────────────────────────────────
static void setCpuExp(Expression e, int ticks);
static void advanceStreet();

// ── Deal animation ────────────────────────────────────────────────────────
static void animDealCard(int tx, int ty, Card c, bool hidden) {
    auto& d = M5Cardputer.Display;
    int sx=107, sy=60;
    // 4 frames from center to target
    int px=sx, py2=sy;
    for(int f=1;f<=4;f++) {
        // Clear previous frame position
        d.fillRect(px,py2,CW+2,CH+2,C_FELT);
        px = sx+(tx-sx)*f/4;
        py2 = sy+(ty-sy)*f/4;
        drawCardTH(px,py2,c,hidden);
        delay(40);
    }
    // Clear the animation card (final draw will be done by caller)
    d.fillRect(px,py2,CW+2,CH+2,C_FELT);
}

// ── Showdown ──────────────────────────────────────────────────────────────
static void showdown() {
    auto& d = M5Cardputer.Display;

    // Animate flipping CPU hole cards
    for(int flip=0;flip<3;flip++) {
        drawCardTH(56,16,cpuHole[0],flip%2==0);
        drawCardTH(84,16,cpuHole[1],flip%2==0);
        delay(150);
    }
    drawCardTH(56,16,cpuHole[0],false);
    drawCardTH(84,16,cpuHole[1],false);
    delay(400);

    // Evaluate both hands
    int playerRank=0, cpuRank=0;
    if(communityCount>=3) {
        Card ph7[7], ch7[7];
        ph7[0]=playerHole[0]; ph7[1]=playerHole[1];
        ch7[0]=cpuHole[0];    ch7[1]=cpuHole[1];
        for(int i=0;i<communityCount;i++) { ph7[2+i]=community[i]; ch7[2+i]=community[i]; }
        if(communityCount+2>=7) {
            playerRank=eval7(ph7);
            cpuRank=eval7(ch7);
        } else {
            // 5 cards total
            Card p5[5], c5[5];
            p5[0]=playerHole[0]; p5[1]=playerHole[1];
            c5[0]=cpuHole[0];    c5[1]=cpuHole[1];
            for(int i=0;i<communityCount;i++){ p5[2+i]=community[i]; c5[2+i]=community[i]; }
            playerRank=evalHand5(p5);
            cpuRank=evalHand5(c5);
        }
    } else {
        // No community: compare hole cards by preFlopStr
        playerRank=preFlopStr(playerHole);
        cpuRank=preFlopStr(cpuHole);
    }

    static const char* HAND_NAMES[]={"High Card","Pair","Two Pair","Three of a Kind","Straight","Flush","Full House","Four of a Kind","Straight Flush","Royal Flush"};

    if(playerRank>cpuRank) {
        playerChips+=pot;
        Persist::credits=playerChips; Persist::updateCredits();
        snprintf(statusMsg,sizeof(statusMsg),"You win! %s +%d",HAND_NAMES[playerRank],pot);
        statusCol=C_GRN; pot=0;
        setCpuExp(EXP_SAD,80); sndWin();
    } else if(cpuRank>playerRank) {
        cpuChips+=pot;
        snprintf(statusMsg,sizeof(statusMsg),"%s wins! %s",CHARS[selectedChar].name,HAND_NAMES[cpuRank]);
        statusCol=C_RED; pot=0;
        setCpuExp(EXP_HAPPY,80); sndLose();
    } else {
        // Split pot
        int half=pot/2;
        playerChips+=half; cpuChips+=(pot-half);
        Persist::credits=playerChips; Persist::updateCredits();
        snprintf(statusMsg,sizeof(statusMsg),"Split pot! %s",HAND_NAMES[playerRank]);
        statusCol=C_GOLD; pot=0;
        setCpuExp(EXP_NEUTRAL,40);
    }

    thState=SHOWDOWN;
    drawHeader();
    drawStatusArea();
    drawActionBar();
    drawOppInfo();
    drawPlayerInfo();
}

// ── CPU AI ────────────────────────────────────────────────────────────────
static void cpuDecide() {
    int str=getCpuHandStrength();
    const Character& ch=CHARS[selectedChar];

    int foldThresh  = 2 + ch.tightness/2;
    int raiseThresh = 5 + (9-ch.aggression)/2;

    int r = random(10);

    if(roundBet>cpuBet) {
        int callAmt=roundBet-cpuBet;
        if(str<foldThresh && r>ch.aggression) {
            cpuFolded=true;
            playerChips+=pot; Persist::credits=playerChips; Persist::updateCredits();
            setCpuExp(EXP_SAD,60);
            snprintf(statusMsg,sizeof(statusMsg),"%s folds. You win!",CHARS[selectedChar].name);
            statusCol=C_GRN; pot=0;
            thState=ROUND_END; sndWin();
        } else if(str>raiseThresh && r<ch.aggression && cpuChips>callAmt+5) {
            int raise=roundBet+5+(ch.aggression>7?5:0);
            if(raise>cpuBet+cpuChips) raise=cpuBet+cpuChips;
            int addAmt=raise-cpuBet;
            if(addAmt>cpuChips) addAmt=cpuChips;
            cpuChips-=addAmt; pot+=addAmt; cpuBet+=addAmt;
            roundBet=cpuBet;
            setCpuExp(EXP_THINK,40);
            snprintf(statusMsg,sizeof(statusMsg),"%s raises to %d.",CHARS[selectedChar].name,cpuBet);
            statusCol=C_GOLD;
            thState=PLAYER_ACT; sndChip();
        } else {
            int actual=callAmt; if(actual>cpuChips) actual=cpuChips;
            cpuChips-=actual; pot+=actual; cpuBet+=actual;
            setCpuExp(EXP_NEUTRAL,30);
            snprintf(statusMsg,sizeof(statusMsg),"%s calls.",CHARS[selectedChar].name);
            statusCol=C_DIM; sndChip();
            advanceStreet();
        }
    } else {
        if(str>raiseThresh && r<ch.aggression+2 && cpuChips>0) {
            int betAmt=min(10,cpuChips);
            cpuChips-=betAmt; pot+=betAmt; cpuBet+=betAmt;
            roundBet=cpuBet;
            setCpuExp(EXP_THINK,40);
            snprintf(statusMsg,sizeof(statusMsg),"%s bets %d.",CHARS[selectedChar].name,betAmt);
            statusCol=C_GOLD;
            thState=PLAYER_ACT; sndChip();
        } else {
            setCpuExp(EXP_NEUTRAL,20);
            snprintf(statusMsg,sizeof(statusMsg),"%s checks.",CHARS[selectedChar].name);
            statusCol=C_DIM;
            advanceStreet();
        }
    }
}

// ── Street advancement ────────────────────────────────────────────────────
static void advanceStreet() {
    static const int CX[5]={43,73,103,133,163};
    roundBet=0; playerBet=0; cpuBet=0; playerActed=false;

    if(communityCount==0) {
        // Deal flop
        community[0]=pullCard(); community[1]=pullCard(); community[2]=pullCard();
        communityCount=3;
        redrawTable();
        // Flash community cards 3 times
        for(int flash=0;flash<3;flash++) {
            for(int i=0;i<3;i++) drawCardTH(CX[i],55,community[i],flash%2==0);
            delay(150);
        }
        for(int i=0;i<3;i++) drawCardTH(CX[i],55,community[i],false);
        statusMsg[0]='\0';
        thState=PLAYER_ACT;
        drawStatusArea(); drawActionBar();
    } else if(communityCount==3) {
        community[3]=pullCard(); communityCount=4;
        for(int flash=0;flash<3;flash++) {
            drawCardTH(CX[3],55,community[3],flash%2==0);
            delay(150);
        }
        drawCardTH(CX[3],55,community[3],false);
        statusMsg[0]='\0';
        thState=PLAYER_ACT;
        drawStatusArea(); drawActionBar();
    } else if(communityCount==4) {
        community[4]=pullCard(); communityCount=5;
        for(int flash=0;flash<3;flash++) {
            drawCardTH(CX[4],55,community[4],flash%2==0);
            delay(150);
        }
        drawCardTH(CX[4],55,community[4],false);
        statusMsg[0]='\0';
        thState=PLAYER_ACT;
        drawStatusArea(); drawActionBar();
    } else {
        showdown();
    }
}

// ── setCpuExp (defined after advanceStreet so we can define it here) ──────
static void setCpuExp(Expression e, int ticks) {
    cpuExp=e; expTimer=ticks; redrawPortrait();
}

// ── New hand ──────────────────────────────────────────────────────────────
static void startNewHand() {
    shuffleDeck();
    // Animate dealing
    playerHole[0]=pullCard(); playerHole[1]=pullCard();
    cpuHole[0]=pullCard();    cpuHole[1]=pullCard();
    communityCount=0; playerFolded=false; cpuFolded=false;
    statusMsg[0]='\0';

    // Blinds: player=SB(5), cpu=BB(10)
    int sb=5, bb=10;
    playerChips-=sb; cpuChips-=bb; pot=sb+bb;
    playerBet=sb; cpuBet=bb; roundBet=bb;
    Persist::credits=playerChips;
    raiseAmount=10;
    handNumber++;
    thState=PLAYER_ACT;
    cpuExp=EXP_NEUTRAL; expTimer=0;

    redrawAll();

    // Deal animations
    animDealCard(90,89,playerHole[0],false);  drawCardTH(90,89,playerHole[0],false);
    animDealCard(56,16,cpuHole[0],true);       drawCardTH(56,16,cpuHole[0],true);
    animDealCard(120,89,playerHole[1],false); drawCardTH(120,89,playerHole[1],false);
    animDealCard(84,16,cpuHole[1],true);       drawCardTH(84,16,cpuHole[1],true);

    drawPlayerInfo();
    drawOppInfo();
    drawActionBar();
    snprintf(statusMsg,sizeof(statusMsg),"Hand %d — Your turn",handNumber);
    statusCol=C_DIM; statusTimer=60;
    drawStatusArea();
}

// ── Public init ───────────────────────────────────────────────────────────
void init() {
    playerChips = Persist::credits;
    cpuChips    = 100;
    pot=0; handNumber=0;
    charSelIdx=0; selectedChar=0;
    thState=SELECT_CHAR;
    blinkTimer=0; eyesBlink=false;
    expTimer=0; cpuExp=EXP_NEUTRAL;
    statusMsg[0]='\0'; statusTimer=0;
    M5Cardputer.Speaker.setVolume(soundVol);
    drawSelectionScreen();
}

// ── Public tick ───────────────────────────────────────────────────────────
bool tick() {
    // Blink timer
    blinkTimer++;
    if(blinkTimer>80){ blinkTimer=0; eyesBlink=true; }
    if(eyesBlink&&blinkTimer>3){ eyesBlink=false; if(thState!=SELECT_CHAR) redrawPortrait(); }

    // Expression timer
    if(expTimer>0){ expTimer--; }
    else if(cpuExp!=EXP_NEUTRAL){ cpuExp=EXP_NEUTRAL; if(thState!=SELECT_CHAR) redrawPortrait(); }

    // Status timer
    if(statusTimer>0){ statusTimer--; }
    else if(statusMsg[0]&&thState!=ROUND_END&&thState!=SHOWDOWN){ statusMsg[0]='\0'; drawStatusArea(); }

    if(M5Cardputer.Keyboard.isChange()&&M5Cardputer.Keyboard.isPressed()) {
        auto st=M5Cardputer.Keyboard.keysState();
        bool act=st.enter;

        for(char c:st.word) {
            if(c=='q'||c=='Q') {
                Persist::credits=playerChips;
                Persist::updateCredits();
                Persist::save();
                return false;
            }
            if(c=='m'||c=='M'){ soundOn=!soundOn; }
            if(c==' ') act=true;

            if(thState==SELECT_CHAR) {
                if(c=='a'||c=='A'){ charSelIdx=(charSelIdx+2)%3; drawSelectionScreen(); }
                if(c=='d'||c=='D'){ charSelIdx=(charSelIdx+1)%3; drawSelectionScreen(); }
            }
            else if(thState==PLAYER_ACT) {
                bool facingBet=(roundBet>playerBet);
                int callAmt=roundBet-playerBet;

                if(c=='a'||c=='A') { // FOLD
                    playerFolded=true;
                    cpuChips+=pot; pot=0;
                    setCpuExp(EXP_HAPPY,80);
                    snprintf(statusMsg,sizeof(statusMsg),"You fold. %s wins!",CHARS[selectedChar].name);
                    statusCol=C_RED; thState=ROUND_END;
                    sndLose();
                    drawStatusArea(); drawActionBar(); drawOppInfo(); drawHeader();
                }
                if(c=='s'||c=='S') { // CHECK or CALL
                    if(facingBet) {
                        int actual=callAmt; if(actual>playerChips) actual=playerChips;
                        playerChips-=actual; pot+=actual; playerBet+=actual;
                        Persist::credits=playerChips;
                        sndChip();
                        drawHeader(); drawPlayerInfo();
                        thState=CPU_ACT; drawActionBar();
                        delay(500);
                        cpuDecide();
                        drawStatusArea(); drawActionBar(); drawOppInfo(); drawPlayerInfo(); drawHeader();
                    } else {
                        playerActed=true;
                        thState=CPU_ACT; drawActionBar();
                        delay(400);
                        cpuDecide();
                        drawStatusArea(); drawActionBar(); drawOppInfo(); drawPlayerInfo(); drawHeader();
                    }
                }
                if(c=='d'||c=='D') { // BET or RAISE
                    int betAmt=raiseAmount;
                    if(facingBet) betAmt+=callAmt;
                    if(betAmt>playerChips) betAmt=playerChips;
                    if(betAmt<=0) break;
                    playerChips-=betAmt; pot+=betAmt; playerBet+=betAmt;
                    if(playerBet>roundBet) roundBet=playerBet;
                    Persist::credits=playerChips;
                    sndChip();
                    drawHeader(); drawPlayerInfo(); drawActionBar();
                    thState=CPU_ACT;
                    delay(600);
                    cpuDecide();
                    drawStatusArea(); drawActionBar(); drawOppInfo(); drawPlayerInfo(); drawHeader();
                }
                if(c=='+'||c=='=') { raiseAmount=min(raiseAmount+5,50); drawActionBar(); }
                if(c=='-')          { raiseAmount=max(raiseAmount-5,5);  drawActionBar(); }
            }
        }

        if(act) {
            if(thState==SELECT_CHAR) {
                selectedChar=charSelIdx;
                startNewHand();
            }
            else if(thState==ROUND_END||thState==SHOWDOWN) {
                if(playerChips<=0) {
                    auto& d=M5Cardputer.Display;
                    d.fillScreen(C_BG);
                    d.setTextSize(1); d.setTextColor(C_RED);
                    const char* t="BROKE! Better luck next time.";
                    d.setCursor((240-d.textWidth(t))/2,55); d.print(t);
                    d.setTextColor(C_DIM);
                    char b[32]; snprintf(b,sizeof(b),"Played %d hands.",handNumber);
                    d.setCursor((240-d.textWidth(b))/2,71); d.print(b);
                    Persist::credits=100; Persist::updateCredits(); Persist::save();
                    delay(3000);
                    return false;
                }
                if(cpuChips<=0) {
                    cpuChips=100;
                    snprintf(statusMsg,sizeof(statusMsg),"Opponent rebuys 100 chips!");
                    statusCol=C_GRN; statusTimer=60;
                }
                startNewHand();
            }
        }
    }

    delay(30);
    return true;
}

} // namespace TexasHoldEm
