#pragma once
#include <M5Cardputer.h>
#include "persist.h"

namespace HiLo {

static const uint16_t C_BG   = 0x0000;
static const uint16_t C_TXT  = 0xFFFF;
static const uint16_t C_DIM  = 0x4208;
static const uint16_t C_GOLD = 0xFEA0;
static const uint16_t C_RED  = 0xF800;
static const uint16_t C_GRN  = 0x07E0;
static const uint16_t C_CYN  = 0x07FF;
static const uint16_t C_CARD = 0xFFFF;
static const uint16_t C_BACK = 0x000F;

// ── Card geometry ──────────────────────────────────────────────────────────
// Header: 0-15  HUD: 16-26  Card: 28-103  Chain:106  Msg:116  Footer:126-134
static const int CW = 54, CH = 76;
static const int CX = (240 - CW) / 2;  // 93
static const int CY = 28;

static const uint16_t SUIT_COL[4] = { 0x2104, C_RED, C_RED, 0x2104 };

struct Card { int val; int suit; };

static void drawSuit(int cx, int cy, int suit, uint16_t col) {
    auto& d = M5Cardputer.Display;
    // scale=2 version: r1=10, tr=18
    switch (suit) {
        case 0: // spade
            d.fillCircle(cx-10,cy-6,10,col); d.fillCircle(cx+10,cy-6,10,col);
            d.fillTriangle(cx-20,cy+5,cx+20,cy+5,cx,cy+18,col);
            d.fillRect(cx-6,cy+20,12,7,col); break;
        case 1: // heart
            d.fillCircle(cx-10,cy-6,10,col); d.fillCircle(cx+10,cy-6,10,col);
            d.fillTriangle(cx-20,cy+5,cx+20,cy+5,cx,cy+19,col); break;
        case 2: // diamond
            d.fillTriangle(cx,cy-18,cx+18,cy,cx-18,cy,col);
            d.fillTriangle(cx-18,cy,cx+18,cy,cx,cy+18,col); break;
        case 3: // club
            d.fillCircle(cx,cy-10,10,col);
            d.fillCircle(cx-10,cy+3,10,col); d.fillCircle(cx+10,cy+3,10,col);
            d.fillRect(cx-6,cy+12,12,7,col); break;
    }
}

static void drawBigCard(Card c, bool hidden=false) {
    auto& d = M5Cardputer.Display;
    if (hidden) {
        d.fillRoundRect(CX,CY,CW,CH,5,C_BACK);
        d.drawRoundRect(CX,CY,CW,CH,5,C_DIM);
        for(int py=CY+5;py<CY+CH-4;py+=5) d.drawFastHLine(CX+4,py,CW-8,0x0018);
        return;
    }
    uint16_t sc = SUIT_COL[c.suit];
    d.fillRoundRect(CX,CY,CW,CH,5,C_CARD);
    d.drawRoundRect(CX,CY,CW,CH,5,sc);
    d.drawRoundRect(CX+1,CY+1,CW-2,CH-2,4,sc);
    char val[4];
    if(c.val==1) strcpy(val,"A"); else if(c.val==11) strcpy(val,"J");
    else if(c.val==12) strcpy(val,"Q"); else if(c.val==13) strcpy(val,"K");
    else snprintf(val,sizeof(val),"%d",c.val);
    d.setTextSize(1); d.setTextColor(sc);
    d.setCursor(CX+4,CY+4); d.print(val);
    drawSuit(CX+CW/2, CY+CH/2, c.suit, sc);
    d.setCursor(CX+CW-d.textWidth(val)-4, CY+CH-11); d.print(val);
}

// ── State ─────────────────────────────────────────────────────────────────
static int   deck[52], deckTop;
static Card  current, next;
static int   bet, pot, chain;
static char  msg[56]; static uint16_t msgCol;
static bool  soundOn=false; static int soundVol=100;
static enum  { BETTING, GUESSING, RESULT } hlState;

static void sndRight() { if(!soundOn)return; M5Cardputer.Speaker.tone(1046,80);delay(90);M5Cardputer.Speaker.tone(1318,120); }
static void sndWrong() { if(!soundOn)return; M5Cardputer.Speaker.tone(300,200); }
static void sndCash()  { if(!soundOn)return; M5Cardputer.Speaker.tone(880,60);delay(70);M5Cardputer.Speaker.tone(1100,60);delay(70);M5Cardputer.Speaker.tone(1320,100); }

static void buildDeck() {
    int k=0;
    for(int s=0;s<4;s++) for(int v=1;v<=13;v++) deck[k++]=v*10+s;
    for(int i=51;i>0;i--){int j=random(i+1);int t=deck[i];deck[i]=deck[j];deck[j]=t;}
    deckTop=0;
}
static Card pullCard() {
    if(deckTop>=52) buildDeck();
    int d=deck[deckTop++]; return {d/10, d%10};
}

static void drawHeader() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,0,240,16,0x0C00); d.setTextSize(1); d.setTextColor(C_GRN);
    const char* t="<<  H I G H  /  L O W  >>";
    d.setCursor((240-d.textWidth(t))/2,4); d.print(t);
}
static void drawHUD() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,16,240,11,C_BG); d.setTextSize(1);
    char l[24],r[24];
    snprintf(l,sizeof(l),"BET:%d  POT:%d",bet,pot);
    snprintf(r,sizeof(r),"CREDITS:%d",Persist::credits);
    d.setTextColor(chain>=2?C_GOLD:C_TXT); d.setCursor(4,17); d.print(l);
    d.setTextColor(C_TXT); d.setCursor(240-d.textWidth(r)-4,17); d.print(r);
}
static void drawChain() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,106,240,10,C_BG);
    if(chain<=0) return;
    d.setTextSize(1); d.setTextColor(chain>=3?C_GOLD:(chain>=2?C_CYN:C_GRN));
    char b[36]; snprintf(b,sizeof(b),"CHAIN x%d  Pot: %d coins",chain,pot);
    d.setCursor((240-d.textWidth(b))/2,107); d.print(b);
}
static void drawMsg() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,117,240,9,C_BG);
    if(!msg[0]) return;
    d.setTextSize(1); d.setTextColor(msgCol);
    d.setCursor((240-d.textWidth(msg))/2,117); d.print(msg);
}
static void drawFooter() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,126,240,9,0x0821);
    d.setTextSize(1); d.setTextColor(0x39E7); d.setCursor(2,127);
    if(hlState==BETTING)         d.print("+/-=BET   SPC=DEAL   Q=MENU");
    else if(hlState==GUESSING)   d.print("H=Higher  L=Lower  C=Cashout  Q=MENU");
    else                         d.print("SPC=AGAIN   Q=MENU");
}

static void fullRedraw() {
    auto& d=M5Cardputer.Display;
    d.fillScreen(C_BG);
    drawHeader(); drawHUD(); drawBigCard(current); drawChain(); drawMsg(); drawFooter();
}

static void startRound() {
    if(Persist::credits<bet){snprintf(msg,sizeof(msg),"Not enough credits!");msgCol=C_RED;drawMsg();return;}
    Persist::credits-=bet;
    Persist::updateCredits();
    pot=bet; chain=0;
    buildDeck(); current=pullCard(); next=pullCard();
    hlState=GUESSING; msg[0]='\0';
    fullRedraw();
}

static void guess(bool wantHigh) {
    auto& d=M5Cardputer.Display;
    // Card flip: shrink then expand
    for(int w=CW;w>4;w-=9){
        d.fillRoundRect(CX+(CW-w)/2,CY,w,CH,4,C_BACK);
        delay(25);
    }
    for(int w=4;w<=CW;w+=9){
        d.fillRect(CX,CY,CW,CH,C_BG);
        d.fillRoundRect(CX+(CW-w)/2,CY,w,CH,4,C_CARD);
        delay(25);
    }
    drawBigCard(next);

    bool tie=(current.val==next.val);
    bool correct=tie || (wantHigh ? next.val>current.val : next.val<current.val);

    if(tie){
        snprintf(msg,sizeof(msg),"Tie! No change (chain x%d)",chain); msgCol=C_CYN;
        current=next; next=pullCard();
        hlState=GUESSING;
        drawHUD(); drawChain(); drawMsg(); drawFooter();
    } else if(correct){
        chain++; pot*=2;
        snprintf(msg,sizeof(msg),"Correct! Pot -> %d",pot); msgCol=C_GRN;
        sndRight();
        d.drawRoundRect(CX,CY,CW,CH,5,C_GOLD); d.drawRoundRect(CX+1,CY+1,CW-2,CH-2,4,C_GOLD);
        delay(200);
        current=next; next=pullCard();
        hlState=GUESSING;
        drawHUD(); drawChain(); drawMsg(); drawFooter();
        drawBigCard(current);
    } else {
        snprintf(msg,sizeof(msg),"Wrong! Lost %d coins.",pot); msgCol=C_RED;
        sndWrong(); chain=0; pot=0;
        hlState=RESULT;
        drawHUD(); drawChain(); drawMsg(); drawFooter();
        if(Persist::credits<=0){
            snprintf(msg,sizeof(msg),"BROKE!  SPC=reset  Q=menu");msgCol=C_RED;drawMsg();
            Persist::credits=100; Persist::save();
        }
    }
}

static void cashOut() {
    if(hlState!=GUESSING||chain==0){
        snprintf(msg,sizeof(msg),"Nothing to cash out!"); msgCol=C_DIM; drawMsg(); return;
    }
    Persist::credits+=pot;
    Persist::updateCredits();
    sndCash();
    snprintf(msg,sizeof(msg),"Cashed out %d coins!",pot); msgCol=C_GOLD;
    pot=0; chain=0; hlState=RESULT;
    drawHUD(); drawChain(); drawMsg(); drawFooter();
}

void init() {
    bet=5; pot=0; chain=0; msg[0]='\0'; msgCol=C_DIM; hlState=BETTING;
    M5Cardputer.Speaker.setVolume(soundVol);
    buildDeck(); current=pullCard(); next=pullCard();
    auto& d=M5Cardputer.Display;
    d.fillScreen(C_BG); drawHeader(); drawHUD();
    drawBigCard(current,true);
    snprintf(msg,sizeof(msg),"Set bet and deal!"); msgCol=C_DIM;
    drawMsg(); drawFooter();
}

bool tick() {
    if(M5Cardputer.Keyboard.isChange()&&M5Cardputer.Keyboard.isPressed()){
        auto st=M5Cardputer.Keyboard.keysState();
        bool act=st.enter;
        for(char c:st.word){
            if(c=='q'||c=='Q') return false;
            if(c=='m'||c=='M'){soundOn=!soundOn;drawFooter();}
            if(c==']'){soundVol=min(soundVol+50,250);M5Cardputer.Speaker.setVolume(soundVol);}
            if(c=='['){soundVol=max(soundVol-50,50);M5Cardputer.Speaker.setVolume(soundVol);}
            if(c==' ') act=true;
            if(hlState==BETTING){
                if(c=='+'||c=='='){bet=min(bet+1,20);drawHUD();}
                if(c=='-')       {bet=max(bet-1,1); drawHUD();}
            } else if(hlState==GUESSING){
                if(c=='h'||c=='H') guess(true);
                if(c=='l'||c=='L') guess(false);
                if(c=='c'||c=='C') cashOut();
            }
        }
        if(act){
            if(hlState==BETTING) startRound();
            else if(hlState==RESULT){
                if(Persist::credits<=0){Persist::credits=100;Persist::save();bet=min(bet,5);}
                hlState=BETTING; msg[0]='\0'; pot=0; chain=0;
                auto& d=M5Cardputer.Display;
                d.fillScreen(C_BG); drawHeader(); drawHUD();
                drawBigCard(current,true);
                snprintf(msg,sizeof(msg),"Set bet and deal!"); msgCol=C_DIM;
                drawMsg(); drawFooter();
            }
        }
    }
    delay(50);
    return true;
}

} // namespace HiLo
