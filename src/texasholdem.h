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
static const uint16_t C_HDRBG = 0x1800;
static const uint16_t C_ACTBG = 0x0821;
static const uint16_t C_BACK  = 0x000F;

// ── Layout ────────────────────────────────────────────────────────────────
// y=0-11  : header (12px)
// y=12-47 : 3 opponent zones, each 80×36
// y=48-85 : community area (cards at y=52, CW=24 CH=28)
// y=86-91 : status row
// y=92-120: player area (cards at y=92 CW=24 CH=28 → bottom=120)
// y=121-134: action bar
static const int OPP_Y  = 12, OPP_H = 36, OPP_W = 80;
static const int COM_Y  = 48;
static const int STS_Y  = 86;
static const int PLY_Y  = 92;
static const int ACT_Y  = 121;

static const int CW  = 24, CH  = 28;   // full cards
static const int MCW = 13, MCH = 15;   // mini cards for opponent zones

// Community card x positions (5 cards × 24 + 4 gaps × 3 = 132, offset=54)
static const int CCX[5] = {54, 81, 108, 135, 162};
static const int CC_Y   = 52;

// Player card positions (2 cards × 24 + 1 gap × 4 = 52, offset=94)
static const int PC_X1 = 94, PC_X2 = 122, PC_Y = 92;

// ── Types ─────────────────────────────────────────────────────────────────
struct Card { int val; int suit; };  // val 1-13 (1=Ace), suit 0-3

enum Expression { EXP_NEUTRAL=0, EXP_HAPPY, EXP_SAD, EXP_THINK, EXP_SHOCK };
enum THState    { PLAYER_ACT=0, CPU_SEQ, SHOWDOWN, ROUND_END };

struct Character {
    const char* name;
    uint16_t skinCol, hairCol, clothCol;
    int aggression;  // 1-9: higher = more likely to bet/raise
    int tightness;   // 1-9: higher = more likely to fold
};

// ── Characters (all 3 always at the table) ────────────────────────────────
static const int NCPU = 3;
static const Character CHARS[NCPU] = {
    { "COWBOY",  0xD6BA, 0x6200, 0xC980, 8, 3 },
    { "SHARK",   0xBDD7, 0x18C3, 0x0010, 4, 8 },
    { "ROOKIE",  0xFEEA, 0xC400, 0x07FF, 5, 2 },
};

static const uint16_t SUIT_COL[4] = { 0x2104, C_RED, C_RED, 0x2104 };

// ── Deck ──────────────────────────────────────────────────────────────────
static Card deck[52];
static int  deckTop;

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

// ── State ─────────────────────────────────────────────────────────────────
static int   playerChips, playerBet;
static bool  playerFolded;
static Card  playerHole[2];

static int        cpuChipsArr[NCPU], cpuBetArr[NCPU];
static bool       cpuFoldedArr[NCPU];
static Card       cpuHoleArr[NCPU][2];
static Expression cpuExpArr[NCPU];
static int        expTimerArr[NCPU];

static int   pot, roundBet, raiseAmount;
static Card  community[5];
static int   communityCount;
static int   handNumber;
static THState thState;
static bool  cpuRaisedThisRound;

static char     statusMsg[56];
static uint16_t statusCol;
static int      statusTimer;
static int      blinkTimer;
static bool     eyesBlink;
static bool     soundOn  = false;
static int      soundVol = 100;

// ── Sound ─────────────────────────────────────────────────────────────────
static void sndWin()  { if(!soundOn)return; M5Cardputer.Speaker.tone(880,80); delay(90); M5Cardputer.Speaker.tone(1100,100); }
static void sndLose() { if(!soundOn)return; M5Cardputer.Speaker.tone(300,200); }
static void sndCard() { if(!soundOn)return; M5Cardputer.Speaker.tone(1000,20); }
static void sndChip() { if(!soundOn)return; M5Cardputer.Speaker.tone(1400,15); }

// ── Hand evaluators ───────────────────────────────────────────────────────
static int evalHand5(Card h[5]) {
    int cnt[14]={}, suits[4]={};
    for(int i=0;i<5;i++){ cnt[h[i].val]++; suits[h[i].suit]++; }
    bool flush=(suits[0]==5||suits[1]==5||suits[2]==5||suits[3]==5);
    int v[5]; for(int i=0;i<5;i++) v[i]=h[i].val;
    for(int i=0;i<4;i++) for(int j=i+1;j<5;j++) if(v[j]<v[i]){int t=v[i];v[i]=v[j];v[j]=t;}
    bool royal=(v[0]==1&&v[1]==10&&v[2]==11&&v[3]==12&&v[4]==13);
    bool straight=royal;
    if(!straight){straight=true;for(int i=0;i<4;i++)if(v[i+1]!=v[i]+1){straight=false;break;}}
    int pairs=0,trips=0,quads=0;
    for(int vv=1;vv<=13;vv++){
        if(cnt[vv]==4)quads++;
        if(cnt[vv]==3)trips++;
        if(cnt[vv]==2)pairs++;
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

static int eval7(Card h[7]) {
    int best=0;
    for(int s1=0;s1<7;s1++) for(int s2=s1+1;s2<7;s2++){
        Card h5[5]; int k=0;
        for(int i=0;i<7;i++) if(i!=s1&&i!=s2) h5[k++]=h[i];
        int r=evalHand5(h5); if(r>best) best=r;
    }
    return best;
}

static int preFlopStr(Card h[2]) {
    int v1=h[0].val, v2=h[1].val;
    if(v1==1) v1=14; if(v2==1) v2=14;
    if(v1<v2){ int t=v1; v1=v2; v2=t; }
    bool suited=(h[0].suit==h[1].suit);
    if(v1==v2) return min(9,4+v1/3);
    if(v1==14&&v2>=10) return 8;
    if(v1==14&&suited) return 6;
    if(v1>=11&&v2>=10) return 7;
    if(v1==14) return 5;
    if(suited&&v1-v2<=2) return 5;
    if(v1>=10&&v2>=8)   return 4;
    return 2;
}

static int getCpuStr(int i) {
    if(communityCount>=3) {
        Card h5[5]={cpuHoleArr[i][0],cpuHoleArr[i][1],community[0],community[1],community[2]};
        return evalHand5(h5);
    }
    return preFlopStr(cpuHoleArr[i]);
}

// ── Drawing: suit symbols ─────────────────────────────────────────────────
static void drawSuitTH(int cx, int cy, int suit, uint16_t col) {
    auto& d = M5Cardputer.Display;
    switch(suit) {
        case 0: d.fillCircle(cx-3,cy-1,3,col); d.fillCircle(cx+3,cy-1,3,col);
                d.fillTriangle(cx-5,cy+1,cx+5,cy+1,cx,cy+6,col);
                d.fillRect(cx-1,cy+5,3,3,col); break;
        case 1: d.fillCircle(cx-3,cy-1,3,col); d.fillCircle(cx+3,cy-1,3,col);
                d.fillTriangle(cx-5,cy+1,cx+5,cy+1,cx,cy+7,col); break;
        case 2: d.fillTriangle(cx,cy-5,cx+5,cy,cx-5,cy,col);
                d.fillTriangle(cx-5,cy,cx+5,cy,cx,cy+5,col); break;
        case 3: d.fillCircle(cx,cy-3,3,col); d.fillCircle(cx-4,cy+1,3,col);
                d.fillCircle(cx+4,cy+1,3,col); d.fillRect(cx-1,cy+3,3,4,col); break;
    }
}

// 5×5 suit for card top-left corner
static void drawSuitSmall(int x, int y, int suit, uint16_t col) {
    auto& d = M5Cardputer.Display;
    switch(suit) {
        case 0: d.fillTriangle(x+2,y,x,y+3,x+4,y+3,col); d.fillRect(x+1,y+3,3,2,col); break;
        case 1: d.fillCircle(x+1,y+1,1,col); d.fillCircle(x+3,y+1,1,col);
                d.fillTriangle(x,y+1,x+4,y+1,x+2,y+4,col); break;
        case 2: d.drawFastHLine(x+2,y,1,col); d.drawFastHLine(x+1,y+1,3,col);
                d.drawFastHLine(x,y+2,5,col); d.drawFastHLine(x+1,y+3,3,col);
                d.drawFastHLine(x+2,y+4,1,col); break;
        case 3: d.fillCircle(x+2,y+1,1,col); d.fillCircle(x+1,y+3,1,col);
                d.fillCircle(x+3,y+3,1,col); d.fillRect(x+1,y+4,3,1,col); break;
    }
}

// ── Drawing: full card (CW×CH) ────────────────────────────────────────────
static void drawCardTH(int x, int y, Card c, bool hidden=false) {
    auto& d = M5Cardputer.Display;
    if(hidden) {
        d.fillRoundRect(x,y,CW,CH,3,C_BACK);
        d.drawRoundRect(x,y,CW,CH,3,C_DIM);
        for(int py=y+4;py<y+CH-3;py+=4) d.drawFastHLine(x+3,py,CW-6,0x0018);
        return;
    }
    uint16_t sc=SUIT_COL[c.suit];
    d.fillRoundRect(x,y,CW,CH,3,0xFFFF);
    d.drawRoundRect(x,y,CW,CH,3,sc);
    char val[4];
    if(c.val==1)       strcpy(val,"A");
    else if(c.val==11) strcpy(val,"J");
    else if(c.val==12) strcpy(val,"Q");
    else if(c.val==13) strcpy(val,"K");
    else               snprintf(val,sizeof(val),"%d",c.val);
    d.setTextSize(1); d.setTextColor(sc);
    d.setCursor(x+2,y+2); d.print(val);
    drawSuitSmall(x+2, y+10, c.suit, sc);
    // Center rank + suit
    int tw=d.textWidth(val);
    d.setCursor(x+(CW-tw)/2, y+5); d.print(val);
    drawSuitTH(x+CW/2, y+19, c.suit, sc);
}

// ── Drawing: mini card (MCW×MCH) for opponent zones ───────────────────────
static void drawMiniCardTH(int x, int y, bool hidden, Card c) {
    auto& d = M5Cardputer.Display;
    if(hidden) {
        d.fillRoundRect(x,y,MCW,MCH,2,C_BACK);
        d.drawRoundRect(x,y,MCW,MCH,2,C_DIM);
        d.drawFastHLine(x+2,y+3,MCW-4,0x0018);
        d.drawFastHLine(x+2,y+6,MCW-4,0x0010);
        d.drawFastHLine(x+2,y+9,MCW-4,0x0018);
    } else {
        uint16_t sc=SUIT_COL[c.suit];
        d.fillRoundRect(x,y,MCW,MCH,2,0xFFFF);
        d.drawRoundRect(x,y,MCW,MCH,2,sc);
        char val[4];
        if(c.val==1)       strcpy(val,"A");
        else if(c.val==11) strcpy(val,"J");
        else if(c.val==12) strcpy(val,"Q");
        else if(c.val==13) strcpy(val,"K");
        else               snprintf(val,sizeof(val),"%d",c.val);
        d.setTextSize(1); d.setTextColor(sc);
        d.setCursor(x+1,y+1); d.print(val);
        drawSuitSmall(x+1, y+8, c.suit, sc);
    }
}

// ── Drawing: opponent portrait (24×33) at (px, py) ───────────────────────
static void drawOppPortrait(int px, int py, int ci, Expression exp, bool blink) {
    auto& d = M5Cardputer.Display;
    d.fillRect(px, py, 24, 33, C_FELT);
    const Character& ch = CHARS[ci];
    int cx = px+12;   // horizontal center
    int cy = py+19;   // vertical center of head

    if(ci==0) { // COWBOY ─ tan skin, wide-brimmed hat, thick mustache
        // Hat crown
        d.fillRect(px+5,  cy-18, 14, 9, 0x6200);
        // Hat brim (wider than crown)
        d.fillRect(px+1,  cy-10, 22, 3, 0x6200);
        // Hat band highlight
        d.drawFastHLine(px+5, cy-10, 14, 0x3100);
        // Head (warm tan)
        d.fillCircle(cx, cy, 9, ch.skinCol);
        // Eyes
        if(!blink) {
            d.fillCircle(cx-3,cy-3,2,0x2104); d.fillCircle(cx+3,cy-3,2,0x2104);
            d.drawPixel(cx-2,cy-4,0xFFFF);    d.drawPixel(cx+4,cy-4,0xFFFF);
        } else {
            d.drawFastHLine(cx-5,cy-3,4,0x2104);
            d.drawFastHLine(cx+1,cy-3,4,0x2104);
        }
        // Bushy mustache
        d.fillRect(cx-5,cy+2,10,3,0x4000);
        d.fillRect(cx-4,cy+4,8,2,0x5000);
        // Mouth (below mustache)
        switch(exp) {
            case EXP_HAPPY: d.drawLine(cx-3,cy+8,cx,cy+6,0x5000); d.drawLine(cx,cy+6,cx+3,cy+8,0x5000); break;
            case EXP_SAD:   d.drawLine(cx-3,cy+6,cx,cy+8,0x5000); d.drawLine(cx,cy+8,cx+3,cy+6,0x5000); break;
            default:        d.drawFastHLine(cx-2,cy+7,4,0x5000); break;
        }
        // Shirt collar (leather vest color)
        d.fillRect(cx-7,py+27,14,6,ch.clothCol);
        d.fillTriangle(cx-7,py+27,cx,py+31,cx-1,py+27,0xC638);
        d.fillTriangle(cx+7,py+27,cx,py+31,cx+1,py+27,0xC638);
    }
    else if(ci==1) { // SHARK ─ pale skin, slicked hair, shades, dark suit
        // Slick dark hair swept back
        d.fillRect(px+3,py+2,18,7,ch.hairCol);
        d.fillTriangle(px+3,py+2,px,py+7,px+3,py+8,ch.hairCol);
        // Head (pale)
        d.fillCircle(cx,cy,9,ch.skinCol);
        // Sunglasses (iconic, flat black lenses)
        d.fillRoundRect(cx-8,cy-4,7,5,1,0x0000);
        d.fillRoundRect(cx+1,cy-4,7,5,1,0x0000);
        d.drawFastHLine(cx-1,cy-2,2,0x4208);  // bridge
        // Thin smirk
        switch(exp) {
            case EXP_HAPPY: d.drawFastHLine(cx-3,cy+5,6,0x4208); d.drawPixel(cx+3,cy+4,0x4208); break;
            case EXP_SAD:   d.drawLine(cx-3,cy+4,cx+3,cy+5,0x4208); break;
            default:        d.drawFastHLine(cx-2,cy+5,5,0x4208); break;
        }
        // Dark suit + white shirt + red tie
        d.fillRect(px+2,py+27,20,6,ch.clothCol);
        d.fillRect(cx-4,py+27,8,6,0xFFFF);
        d.fillRect(cx-1,py+27,2,6,C_RED);
        d.fillTriangle(px+2,py+27,cx-1,py+31,cx-1,py+27,0xFFFF);
        d.fillTriangle(px+22,py+27,cx+1,py+31,cx+1,py+27,0xFFFF);
    }
    else { // ROOKIE ─ light skin, spiky blonde hair, big blue eyes
        // Spiky hair (3 spikes above head)
        d.fillRect(px+3,py+6,18,5,ch.hairCol);
        d.fillTriangle(cx-7,py+6,cx-5,py+0,cx-2,py+6,ch.hairCol);
        d.fillTriangle(cx-1,py+5,cx+1,py-1,cx+4,py+5,ch.hairCol);
        d.fillTriangle(cx+4,py+6,cx+7,py+1,cx+9,py+6,ch.hairCol);
        // Head
        d.fillCircle(cx,cy,9,ch.skinCol);
        // Big wide eyes (blue iris, white sclera)
        if(!blink) {
            d.fillCircle(cx-3,cy-3,3,0xFFFF); d.fillCircle(cx+3,cy-3,3,0xFFFF);
            d.fillCircle(cx-3,cy-3,2,0x001F); d.fillCircle(cx+3,cy-3,2,0x001F);
            d.fillCircle(cx-3,cy-3,1,0x0000); d.fillCircle(cx+3,cy-3,1,0x0000);
            d.drawPixel(cx-2,cy-4,0xFFFF);    d.drawPixel(cx+4,cy-4,0xFFFF);
        } else {
            d.fillRect(cx-6,cy-4,6,2,ch.skinCol);
            d.fillRect(cx+1,cy-4,6,2,ch.skinCol);
        }
        // Mouth
        switch(exp) {
            case EXP_HAPPY: d.drawLine(cx-3,cy+6,cx,cy+4,0x5000); d.drawLine(cx,cy+4,cx+3,cy+6,0x5000); break;
            case EXP_SAD:   d.drawLine(cx-3,cy+4,cx,cy+6,0x5000); d.drawLine(cx,cy+6,cx+3,cy+4,0x5000); break;
            default:        d.drawLine(cx-3,cy+5,cx+3,cy+5,0x5000); break;
        }
        // Hoodie
        d.fillRect(px+2,py+27,20,6,ch.clothCol);
        d.fillRect(cx-5,py+27,10,6,(uint16_t)(ch.clothCol>>1));
    }
}

// ── Drawing: one opponent zone ─────────────────────────────────────────────
static void drawOppZone(int i, bool highlight=false) {
    auto& d = M5Cardputer.Display;
    int zx = i * OPP_W;
    d.fillRect(zx, OPP_Y, OPP_W-1, OPP_H, C_FELT);
    uint16_t border = highlight ? C_GOLD : C_DIM;
    d.drawRect(zx, OPP_Y, OPP_W-1, OPP_H, border);

    // Portrait (24×33) at zx+1, OPP_Y+2
    drawOppPortrait(zx+1, OPP_Y+2, i, cpuExpArr[i], eyesBlink && !cpuFoldedArr[i]);

    int rx = zx+27;  // right area start x

    if(cpuFoldedArr[i]) {
        d.setTextSize(1); d.setTextColor(C_DIM);
        d.setCursor(rx, OPP_Y+3);  d.print(CHARS[i].name);
        d.setCursor(rx, OPP_Y+14); d.print("FOLD");
        return;
    }
    d.setTextSize(1);
    d.setTextColor(CHARS[i].clothCol);
    d.setCursor(rx, OPP_Y+3); d.print(CHARS[i].name);
    d.setTextColor(C_GOLD);
    char chips[12]; snprintf(chips,sizeof(chips),"%d",cpuChipsArr[i]);
    d.setCursor(rx, OPP_Y+13); d.print(chips);
    if(cpuBetArr[i]>0) {
        d.setTextColor(0xFD20);
        char bet[10]; snprintf(bet,sizeof(bet),"B:%d",cpuBetArr[i]);
        d.setCursor(rx, OPP_Y+22); d.print(bet);
    }
    // Mini cards at zone bottom
    bool reveal = (thState==SHOWDOWN);
    int cardY = OPP_Y + OPP_H - MCH - 1;  // = 32 → bottom=47 ✓
    drawMiniCardTH(rx,       cardY, !reveal, cpuHoleArr[i][0]);
    drawMiniCardTH(rx+MCW+2, cardY, !reveal, cpuHoleArr[i][1]);
}

// ── Drawing: community area ────────────────────────────────────────────────
static void drawCommunityArea() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0, COM_Y, 240, STS_Y-COM_Y, C_FELT);
    // Pot label centered
    d.setTextSize(1); d.setTextColor(C_GOLD);
    char potstr[20]; snprintf(potstr,sizeof(potstr),"POT: %d",pot);
    d.setCursor((240-d.textWidth(potstr))/2, COM_Y+2); d.print(potstr);
    // Five card slots
    for(int i=0;i<5;i++) {
        if(i<communityCount)
            drawCardTH(CCX[i], CC_Y, community[i], false);
        else
            d.drawRoundRect(CCX[i], CC_Y, CW, CH, 3, C_DIM);
    }
}

// ── Drawing: player area ───────────────────────────────────────────────────
static void drawPlayerArea() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0, PLY_Y, 240, ACT_Y-PLY_Y, C_FELT);
    drawCardTH(PC_X1, PC_Y, playerHole[0], playerFolded);
    drawCardTH(PC_X2, PC_Y, playerHole[1], playerFolded);
    // Left: label + chips
    d.setTextSize(1);
    d.setTextColor(C_DIM); d.setCursor(4, PC_Y+4);  d.print("YOU");
    d.setTextColor(C_GOLD);
    char chs[16]; snprintf(chs,sizeof(chs),"$%d",playerChips);
    d.setCursor(4, PC_Y+14); d.print(chs);
    // Right: bet amount
    if(playerBet>0) {
        d.setTextColor(0xFD20);
        char bs[12]; snprintf(bs,sizeof(bs),"B:%d",playerBet);
        d.setCursor(240-d.textWidth(bs)-4, PC_Y+4); d.print(bs);
    }
}

// ── Drawing: status row ────────────────────────────────────────────────────
static void drawStatusRow() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0, STS_Y, 240, PLY_Y-STS_Y, C_FELT);
    if(!statusMsg[0]) return;
    d.setTextSize(1); d.setTextColor(statusCol);
    d.setCursor((240-d.textWidth(statusMsg))/2, STS_Y+1); d.print(statusMsg);
}

// ── Drawing: header ────────────────────────────────────────────────────────
static void drawHeader() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0,0,240,12,C_HDRBG);
    d.setTextSize(1); d.setTextColor(0xF81F);
    d.setCursor(4,2); d.print("TEXAS HOLD'EM");
    d.setTextColor(C_GOLD);
    char hstr[20]; snprintf(hstr,sizeof(hstr),"Hand:%d",handNumber);
    d.setCursor(240-d.textWidth(hstr)-4,2); d.print(hstr);
    d.drawFastHLine(0,11,240,0x3000);
}

// ── Drawing: action bar ────────────────────────────────────────────────────
static void drawActionBar() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0,ACT_Y,240,135-ACT_Y,C_ACTBG);
    d.drawFastHLine(0,ACT_Y,240,0x3000);
    d.setTextSize(1); d.setTextColor(0x39E7);
    if(thState==PLAYER_ACT) {
        bool facing=(roundBet>playerBet);
        if(facing) {
            char buf[48]; snprintf(buf,sizeof(buf),"A=fold  S=call(%d)  D=raise  +/-=sz",roundBet-playerBet);
            d.setCursor(2,ACT_Y+3); d.print(buf);
        } else {
            d.setCursor(2,ACT_Y+3); d.print("A=fold  S=check  D=bet  +/-=sz  Q=quit");
        }
    } else if(thState==SHOWDOWN||thState==ROUND_END) {
        d.setCursor(2,ACT_Y+3); d.print("SPC=next hand  Q=menu");
    } else {
        d.setCursor(2,ACT_Y+3); d.print("Q=menu");
    }
}

// ── Full redraw ────────────────────────────────────────────────────────────
static void redrawAll() {
    auto& d = M5Cardputer.Display;
    d.fillScreen(C_BG);
    d.fillRect(0, OPP_Y, 240, ACT_Y-OPP_Y, C_FELT);
    drawHeader();
    for(int i=0;i<NCPU;i++) drawOppZone(i);
    drawCommunityArea();
    drawStatusRow();
    drawPlayerArea();
    drawActionBar();
}

// ── Forward declarations ──────────────────────────────────────────────────
static void advanceStreet();
static void showdown();

// ── CPU expression helper ─────────────────────────────────────────────────
static void setCpuExp(int i, Expression e, int ticks) {
    cpuExpArr[i]=e; expTimerArr[i]=ticks;
    drawOppZone(i);
}

// ── CPU decision for one player ───────────────────────────────────────────
static void cpuDecideFor(int i) {
    if(cpuFoldedArr[i]) return;
    const Character& ch=CHARS[i];
    int str=getCpuStr(i);
    int foldThresh  = 2+ch.tightness/2;
    int raiseThresh = 5+(9-ch.aggression)/2;
    int r=random(10);

    if(roundBet>cpuBetArr[i]) {
        int callAmt=roundBet-cpuBetArr[i];
        if(str<foldThresh && r>ch.aggression) {
            cpuFoldedArr[i]=true;
            setCpuExp(i,EXP_SAD,60);
            snprintf(statusMsg,sizeof(statusMsg),"%s folds.",CHARS[i].name);
            statusCol=C_DIM;
        } else if(str>raiseThresh && r<ch.aggression && cpuChipsArr[i]>callAmt+5) {
            int raise=roundBet+5+(ch.aggression>6?5:0);
            int addAmt=raise-cpuBetArr[i]; if(addAmt>cpuChipsArr[i]) addAmt=cpuChipsArr[i];
            cpuChipsArr[i]-=addAmt; pot+=addAmt; cpuBetArr[i]+=addAmt; roundBet=cpuBetArr[i];
            setCpuExp(i,EXP_THINK,40);
            snprintf(statusMsg,sizeof(statusMsg),"%s raises to %d!",CHARS[i].name,cpuBetArr[i]);
            statusCol=C_GOLD; cpuRaisedThisRound=true; sndChip();
        } else {
            int actual=min(callAmt,cpuChipsArr[i]);
            cpuChipsArr[i]-=actual; pot+=actual; cpuBetArr[i]+=actual;
            setCpuExp(i,EXP_NEUTRAL,20);
            snprintf(statusMsg,sizeof(statusMsg),"%s calls.",CHARS[i].name);
            statusCol=C_DIM; sndChip();
        }
    } else {
        if(str>raiseThresh && r<ch.aggression && cpuChipsArr[i]>0) {
            int betAmt=min(10,cpuChipsArr[i]);
            cpuChipsArr[i]-=betAmt; pot+=betAmt; cpuBetArr[i]+=betAmt; roundBet=cpuBetArr[i];
            setCpuExp(i,EXP_THINK,40);
            snprintf(statusMsg,sizeof(statusMsg),"%s bets %d.",CHARS[i].name,betAmt);
            statusCol=C_GOLD; cpuRaisedThisRound=true; sndChip();
        } else {
            setCpuExp(i,EXP_NEUTRAL,20);
            snprintf(statusMsg,sizeof(statusMsg),"%s checks.",CHARS[i].name);
            statusCol=C_DIM;
        }
    }
}

// ── Run all CPU decisions in sequence ─────────────────────────────────────
static void runCpuSeq() {
    cpuRaisedThisRound=false;
    for(int i=0;i<NCPU;i++) {
        if(cpuFoldedArr[i]) continue;
        setCpuExp(i,EXP_THINK,30);
        delay(380);
        cpuDecideFor(i);
        drawOppZone(i); drawHeader(); drawCommunityArea(); drawStatusRow();
        delay(180);
    }
    // Count active players
    int active = playerFolded ? 0 : 1;
    for(int i=0;i<NCPU;i++) if(!cpuFoldedArr[i]) active++;

    if(active<=1) {
        if(!playerFolded) {
            playerChips+=pot; Persist::credits=playerChips; Persist::updateCredits();
            snprintf(statusMsg,sizeof(statusMsg),"All fold! You win %d!",pot); pot=0;
            statusCol=C_GRN; sndWin();
        } else {
            for(int i=0;i<NCPU;i++) if(!cpuFoldedArr[i]) {
                cpuChipsArr[i]+=pot; pot=0;
                snprintf(statusMsg,sizeof(statusMsg),"%s wins the pot!",CHARS[i].name);
                statusCol=C_RED; sndLose(); break;
            }
        }
        thState=ROUND_END;
        drawHeader(); drawCommunityArea(); drawStatusRow(); drawActionBar();
        return;
    }

    if(cpuRaisedThisRound && !playerFolded && playerBet<roundBet) {
        thState=PLAYER_ACT;
        snprintf(statusMsg,sizeof(statusMsg),"Respond to raise...");
        statusCol=C_GOLD; statusTimer=0;
        drawStatusRow(); drawActionBar();
    } else {
        advanceStreet();
    }
}

// ── Street advancement ────────────────────────────────────────────────────
static void advanceStreet() {
    roundBet=0; playerBet=0;
    for(int i=0;i<NCPU;i++) cpuBetArr[i]=0;

    if(communityCount==0) {
        // Deal flop
        community[0]=pullCard(); community[1]=pullCard(); community[2]=pullCard();
        communityCount=3;
        drawCommunityArea();
        for(int f=0;f<3;f++) { for(int k=0;k<3;k++) drawCardTH(CCX[k],CC_Y,community[k],f%2==0); delay(130); }
        for(int k=0;k<3;k++) { drawCardTH(CCX[k],CC_Y,community[k],false); sndCard(); delay(40); }
    } else if(communityCount==3) {
        community[3]=pullCard(); communityCount=4;
        for(int f=0;f<3;f++) { drawCardTH(CCX[3],CC_Y,community[3],f%2==0); delay(130); }
        drawCardTH(CCX[3],CC_Y,community[3],false); sndCard();
    } else if(communityCount==4) {
        community[4]=pullCard(); communityCount=5;
        for(int f=0;f<3;f++) { drawCardTH(CCX[4],CC_Y,community[4],f%2==0); delay(130); }
        drawCardTH(CCX[4],CC_Y,community[4],false); sndCard();
    } else {
        showdown(); return;
    }
    statusMsg[0]='\0'; thState=PLAYER_ACT;
    drawHeader(); drawCommunityArea(); drawStatusRow(); drawActionBar(); drawPlayerArea();
}

// ── Showdown ──────────────────────────────────────────────────────────────
static void showdown() {
    static const char* HN[]={"Hi Card","Pair","2 Pair","3-Kind","Straight","Flush","Full Hse","4-Kind","Str.Flush","Royal!"};
    // Reveal CPU cards
    thState=SHOWDOWN;
    for(int i=0;i<NCPU;i++) drawOppZone(i);
    delay(500);

    // Evaluate all hands
    int playerRank = playerFolded ? -1 : 0;
    int cpuRank[NCPU] = {};
    if(communityCount>=5) {
        if(!playerFolded) {
            Card ph7[7]; ph7[0]=playerHole[0]; ph7[1]=playerHole[1];
            for(int j=0;j<5;j++) ph7[2+j]=community[j];
            playerRank=eval7(ph7);
        }
        for(int i=0;i<NCPU;i++) {
            if(!cpuFoldedArr[i]) {
                Card ch7[7]; ch7[0]=cpuHoleArr[i][0]; ch7[1]=cpuHoleArr[i][1];
                for(int j=0;j<5;j++) ch7[2+j]=community[j];
                cpuRank[i]=eval7(ch7);
            } else cpuRank[i]=-1;
        }
    } else {
        if(!playerFolded) playerRank=preFlopStr(playerHole);
        for(int i=0;i<NCPU;i++) cpuRank[i]=cpuFoldedArr[i]?-1:preFlopStr(cpuHoleArr[i]);
    }

    // Find best rank
    int best=playerRank;
    for(int i=0;i<NCPU;i++) if(cpuRank[i]>best) best=cpuRank[i];

    // Count winners and split pot
    int winners=0;
    bool playerWon=(!playerFolded && playerRank==best);
    if(playerWon) winners++;
    for(int i=0;i<NCPU;i++) if(!cpuFoldedArr[i]&&cpuRank[i]==best) winners++;
    if(winners==0) winners=1;
    int share=pot/winners;

    if(playerWon) {
        playerChips+=share; Persist::credits=playerChips; Persist::updateCredits();
        int rank=min(best,9);
        snprintf(statusMsg,sizeof(statusMsg),"You win %d! %s",share,HN[rank]);
        statusCol=C_GOLD; sndWin();
        for(int i=0;i<NCPU;i++) {
            if(!cpuFoldedArr[i]&&cpuRank[i]==best) setCpuExp(i,EXP_NEUTRAL,20);
            else if(!cpuFoldedArr[i]) setCpuExp(i,EXP_SAD,80);
        }
    } else {
        for(int i=0;i<NCPU;i++) {
            if(!cpuFoldedArr[i]&&cpuRank[i]==best) {
                cpuChipsArr[i]+=share; setCpuExp(i,EXP_HAPPY,80);
            } else if(!cpuFoldedArr[i]) {
                setCpuExp(i,EXP_SAD,60);
            }
        }
        for(int i=0;i<NCPU;i++) if(!cpuFoldedArr[i]&&cpuRank[i]==best) {
            int rank=min(best,9);
            snprintf(statusMsg,sizeof(statusMsg),"%s wins! %s",CHARS[i].name,HN[rank]); break;
        }
        statusCol=C_RED; sndLose();
    }
    pot=0;
    drawHeader(); drawCommunityArea(); drawStatusRow(); drawActionBar();
    for(int i=0;i<NCPU;i++) drawOppZone(i);
    drawPlayerArea();
}

// ── Start new hand ────────────────────────────────────────────────────────
static void startNewHand() {
    shuffleDeck();
    playerHole[0]=pullCard(); playerHole[1]=pullCard();
    for(int i=0;i<NCPU;i++) { cpuHoleArr[i][0]=pullCard(); cpuHoleArr[i][1]=pullCard(); }
    communityCount=0; playerFolded=false;
    for(int i=0;i<NCPU;i++) { cpuFoldedArr[i]=false; cpuBetArr[i]=0; cpuExpArr[i]=EXP_NEUTRAL; }
    statusMsg[0]='\0'; cpuRaisedThisRound=false;

    // Rebuy any broke CPUs
    for(int i=0;i<NCPU;i++) if(cpuChipsArr[i]<=0) cpuChipsArr[i]=100;

    // CPU0=SB(5), CPU1=BB(10), player faces BB
    pot=0;
    cpuChipsArr[0]-=5;  pot+=5;  cpuBetArr[0]=5;
    cpuChipsArr[1]-=10; pot+=10; cpuBetArr[1]=10;
    roundBet=10; playerBet=0; raiseAmount=10;
    handNumber++; thState=PLAYER_ACT;

    redrawAll();
    sndCard();

    snprintf(statusMsg,sizeof(statusMsg),"Hand %d — Blinds: 5/10",handNumber);
    statusCol=C_DIM; statusTimer=50;
    drawStatusRow();
}

// ── init ──────────────────────────────────────────────────────────────────
void init() {
    playerChips=Persist::credits;
    for(int i=0;i<NCPU;i++) { cpuChipsArr[i]=100; cpuBetArr[i]=0; cpuFoldedArr[i]=false; cpuExpArr[i]=EXP_NEUTRAL; expTimerArr[i]=0; }
    pot=0; handNumber=0; communityCount=0;
    statusMsg[0]='\0'; statusTimer=0; statusCol=C_DIM;
    blinkTimer=0; eyesBlink=false;
    M5Cardputer.Speaker.setVolume(soundVol);
    startNewHand();
}

// ── tick ──────────────────────────────────────────────────────────────────
bool tick() {
    // Blink animation
    blinkTimer++;
    if(blinkTimer>80){ blinkTimer=0; eyesBlink=true; }
    if(eyesBlink&&blinkTimer>3){ eyesBlink=false; for(int i=0;i<NCPU;i++) if(!cpuFoldedArr[i]) drawOppZone(i); }

    // Expression timers decay to NEUTRAL
    for(int i=0;i<NCPU;i++) {
        if(expTimerArr[i]>0) expTimerArr[i]--;
        else if(cpuExpArr[i]!=EXP_NEUTRAL){ cpuExpArr[i]=EXP_NEUTRAL; if(!cpuFoldedArr[i]) drawOppZone(i); }
    }

    // Status timer
    if(statusTimer>0) statusTimer--;
    else if(statusMsg[0]&&thState==PLAYER_ACT){ statusMsg[0]='\0'; drawStatusRow(); }

    if(M5Cardputer.Keyboard.isChange()&&M5Cardputer.Keyboard.isPressed()) {
        auto st=M5Cardputer.Keyboard.keysState();
        bool act=st.enter;

        for(char c:st.word) {
            if(c=='q'||c=='Q') {
                Persist::credits=playerChips; Persist::updateCredits(); Persist::save();
                return false;
            }
            if(c=='m'||c=='M') soundOn=!soundOn;
            if(c==' ') act=true;

            if(thState==PLAYER_ACT) {
                bool facing=(roundBet>playerBet);
                int callAmt=roundBet-playerBet;

                if(c=='a'||c=='A') { // FOLD
                    playerFolded=true;
                    int cpuActive=0, lastCpu=-1;
                    for(int i=0;i<NCPU;i++) if(!cpuFoldedArr[i]){ cpuActive++; lastCpu=i; }
                    if(cpuActive==1) {
                        cpuChipsArr[lastCpu]+=pot; pot=0;
                        setCpuExp(lastCpu,EXP_HAPPY,80);
                        snprintf(statusMsg,sizeof(statusMsg),"You fold. %s wins!",CHARS[lastCpu].name);
                        statusCol=C_RED; thState=ROUND_END; sndLose();
                        drawHeader(); drawStatusRow(); drawActionBar(); drawPlayerArea();
                    } else {
                        snprintf(statusMsg,sizeof(statusMsg),"You fold.");
                        statusCol=C_RED; drawStatusRow(); drawPlayerArea();
                        delay(250); runCpuSeq();
                    }
                }
                if(c=='s'||c=='S') { // CHECK or CALL
                    if(facing) {
                        int actual=min(callAmt,playerChips);
                        playerChips-=actual; pot+=actual; playerBet+=actual;
                        Persist::credits=playerChips;
                        sndChip(); drawHeader(); drawPlayerArea();
                    }
                    thState=CPU_SEQ; drawActionBar();
                    delay(280); runCpuSeq();
                }
                if(c=='d'||c=='D') { // BET or RAISE
                    int betAmt=raiseAmount; if(facing) betAmt+=callAmt;
                    if(betAmt>playerChips) betAmt=playerChips;
                    if(betAmt>0) {
                        playerChips-=betAmt; pot+=betAmt; playerBet+=betAmt;
                        if(playerBet>roundBet) roundBet=playerBet;
                        Persist::credits=playerChips;
                        sndChip(); drawHeader(); drawPlayerArea();
                        thState=CPU_SEQ; drawActionBar();
                        delay(450); runCpuSeq();
                    }
                }
                if(c=='+'||c=='=') { raiseAmount=min(raiseAmount+5,50); drawActionBar(); }
                if(c=='-')          { raiseAmount=max(raiseAmount-5,5);  drawActionBar(); }
            }
        }

        if(act&&(thState==ROUND_END||thState==SHOWDOWN)) {
            if(playerChips<=0) {
                playerChips=100; Persist::credits=100; Persist::updateCredits(); Persist::save();
            }
            startNewHand();
        }
    }

    delay(30);
    return true;
}

} // namespace TexasHoldEm
