#pragma once
#include <M5Cardputer.h>
#include <math.h>
#include "persist.h"

namespace Roulette {

static const uint16_t C_BG   = 0x0000;
static const uint16_t C_TXT  = 0xFFFF;
static const uint16_t C_DIM  = 0x4208;
static const uint16_t C_GOLD = 0xFEA0;
static const uint16_t C_RED  = 0xF800;
static const uint16_t C_GRN  = 0x07E0;
static const uint16_t C_BLK  = 0x2104;
static const uint16_t C_MGN  = 0xF81F;
static const uint16_t C_ZERO = 0x03E0;

// European roulette clockwise pocket order
static const int WHEEL_ORDER[37] = {
    0,32,15,19,4,21,2,25,17,34,6,27,13,36,11,30,8,23,10,5,24,16,33,1,20,14,31,9,22,18,29,7,28,12,35,3,26
};
static const int RED_NUMS[18] = {1,3,5,7,9,12,14,16,18,19,21,23,25,27,30,32,34,36};

static bool isRed(int n) {
    if(n==0) return false;
    for(int i=0;i<18;i++) if(RED_NUMS[i]==n) return true;
    return false;
}

// ── Elliptical wheel geometry ─────────────────────────────────────────────
// Layout: header 0-15, HUD 16-26, wheel 29-111, msg 113-121, footer 122-134
// Ellipse: center (120,70), outer semi-axes a=108 h, b=40 v
// → top=30, bottom=110, left=12, right=228 — fits the 240×135 screen perfectly

static const int   WCX  = 120, WCY = 70;
static const int   WR_OA = 108, WR_OB = 40;   // outer semi-axes
static const int   WR_IA = 42,  WR_IB = 16;   // inner (hub) semi-axes
static const int   WR_NA = 85,  WR_NB = 31;   // number label position

static const float TWO_PI_F = 6.2831853f;
static const float SEG = TWO_PI_F / 37.0f;

static void drawWheel(float angle, int highlight=-1) {
    auto& d = M5Cardputer.Display;

    for(int i=0;i<37;i++){
        float a0=angle+i*SEG, a1=a0+SEG;
        int n=WHEEL_ORDER[i];
        uint16_t col=(highlight==i)?C_GOLD:((n==0)?C_ZERO:(isRed(n)?C_RED:C_BLK));
        // Fill segment with 4 quads
        for(int s=0;s<4;s++){
            float sa=a0+s*(a1-a0)/4.0f, sb=a0+(s+1)*(a1-a0)/4.0f;
            int ix0=WCX+(int)(WR_IA*cosf(sa)), iy0=WCY+(int)(WR_IB*sinf(sa));
            int ix1=WCX+(int)(WR_IA*cosf(sb)), iy1=WCY+(int)(WR_IB*sinf(sb));
            int ox0=WCX+(int)(WR_OA*cosf(sa)), oy0=WCY+(int)(WR_OB*sinf(sa));
            int ox1=WCX+(int)(WR_OA*cosf(sb)), oy1=WCY+(int)(WR_OB*sinf(sb));
            d.fillTriangle(ix0,iy0,ix1,iy1,ox0,oy0,col);
            d.fillTriangle(ix1,iy1,ox0,oy0,ox1,oy1,col);
        }
        // Segment divider
        int lxi=WCX+(int)(WR_IA*cosf(a0)), lyi=WCY+(int)(WR_IB*sinf(a0));
        int lxo=WCX+(int)(WR_OA*cosf(a0)), lyo=WCY+(int)(WR_OB*sinf(a0));
        d.drawLine(lxi,lyi,lxo,lyo,C_DIM);
    }

    // Hub and borders using M5GFX ellipse primitives
    d.fillEllipse(WCX,WCY,WR_IA-1,WR_IB-1,0x0821);
    d.drawEllipse(WCX,WCY,WR_IA,WR_IB,C_GOLD);
    d.drawEllipse(WCX,WCY,WR_OA,WR_OB,C_GOLD);

    // Number labels
    d.setTextSize(1);
    for(int i=0;i<37;i++){
        float amid=angle+i*SEG+SEG*0.5f;
        int nx=WCX+(int)(WR_NA*cosf(amid));
        int ny=WCY+(int)(WR_NB*sinf(amid));
        int n=WHEEL_ORDER[i];
        char buf[4]; snprintf(buf,sizeof(buf),"%d",n);
        d.setTextColor(C_TXT);
        d.setCursor(nx-d.textWidth(buf)/2, ny-4);
        d.print(buf);
    }

    // Pointer: downward triangle at top of ellipse
    int py=WCY-WR_OB;
    d.fillTriangle(WCX,py-4, WCX-5,py+4, WCX+5,py+4, C_GOLD);
}

// ── Bet types ─────────────────────────────────────────────────────────────
enum BetType { BET_RED=0,BET_BLACK,BET_ODD,BET_EVEN,BET_LOW,BET_HIGH,BET_DOZEN1,BET_DOZEN2,BET_DOZEN3,BET_NTYPES };
static const char* BET_NAMES[BET_NTYPES]={"RED","BLACK","ODD","EVEN","1-18","19-36","1-12","13-24","25-36"};
static const int   BET_PAY [BET_NTYPES]={  2,     2,    2,    2,    2,     2,      3,      3,      3};

static bool betWins(int t, int n){
    if(n==0) return false;
    switch(t){
        case BET_RED:    return isRed(n);
        case BET_BLACK:  return !isRed(n);
        case BET_ODD:    return n%2==1;
        case BET_EVEN:   return n%2==0;
        case BET_LOW:    return n>=1&&n<=18;
        case BET_HIGH:   return n>=19&&n<=36;
        case BET_DOZEN1: return n>=1&&n<=12;
        case BET_DOZEN2: return n>=13&&n<=24;
        case BET_DOZEN3: return n>=25&&n<=36;
    }
    return false;
}

// ── State ─────────────────────────────────────────────────────────────────
static int   bet, betType, result;
static float wheelAngle, maxSpeed;
static int   spinTicks, totalTicks;
static char  msg[56]; static uint16_t msgCol;
static bool  soundOn=false; static int soundVol=100;
static enum  { BETTING, SPINNING, RESULT } rlState;

static void sndTick() { if(!soundOn)return; M5Cardputer.Speaker.tone(700+(int)(maxSpeed*300),12); }
static void sndWin()  { if(!soundOn)return; M5Cardputer.Speaker.tone(880,80);delay(90);M5Cardputer.Speaker.tone(1100,120); }
static void sndLose() { if(!soundOn)return; M5Cardputer.Speaker.tone(300,200); }

static void drawHeader() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,0,240,16,0x1818); d.setTextSize(1); d.setTextColor(C_MGN);
    const char* t="<<  R O U L E T T E  >>";
    d.setCursor((240-d.textWidth(t))/2,4); d.print(t);
}
static void drawHUD() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,16,240,12,C_BG); d.setTextSize(1);
    char l[30],r[22];
    snprintf(l,sizeof(l),"BET:%d on %s",bet,BET_NAMES[betType]);
    snprintf(r,sizeof(r),"CREDITS:%d",Persist::credits);
    uint16_t lc=(betType==BET_RED)?C_RED:(betType==BET_BLACK?C_DIM:C_TXT);
    d.setTextColor(lc); d.setCursor(4,17); d.print(l);
    d.setTextColor(C_TXT); d.setCursor(240-d.textWidth(r)-4,17); d.print(r);
}
static void drawMsg() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,112,240,10,C_BG);
    if(!msg[0]) return;
    d.setTextSize(1); d.setTextColor(msgCol);
    d.setCursor((240-d.textWidth(msg))/2,113); d.print(msg);
}
static void drawFooter() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,122,240,13,0x0821);
    d.setTextSize(1); d.setTextColor(0x39E7); d.setCursor(2,124);
    if(rlState==RESULT) d.print("SPC=AGAIN  A/D=BetType  +/-=Bet  Q=MENU");
    else                d.print("A/D=BetType  +/-=Bet  SPC=SPIN  Q=MENU");
}

static void redrawAll() {
    auto& d=M5Cardputer.Display;
    d.fillScreen(C_BG);
    drawHeader(); drawHUD();
    drawWheel(wheelAngle);
    drawMsg(); drawFooter();
}

// Find pocket index currently under the pointer (angle = -PI/2)
static int pocketUnderPointer() {
    float ptr = -TWO_PI_F/4.0f;
    int best=0; float bestDiff=999.f;
    for(int i=0;i<37;i++){
        float mid = wheelAngle + i*SEG + SEG*0.5f;
        float diff = fmodf(mid - ptr, TWO_PI_F);
        if(diff<0) diff+=TWO_PI_F;
        if(diff>TWO_PI_F/2.0f) diff=TWO_PI_F-diff;
        if(diff<bestDiff){bestDiff=diff;best=i;}
    }
    return best;
}

static void startSpin() {
    if(Persist::credits<bet){snprintf(msg,sizeof(msg),"Not enough credits!");msgCol=C_RED;drawMsg();return;}
    Persist::credits-=bet;
    Persist::updateCredits();
    totalTicks = 100 + random(60);
    maxSpeed   = 0.42f + random(20)*0.012f;
    spinTicks  = 0;
    rlState    = SPINNING;
    msg[0]='\0'; drawHUD(); drawMsg();
}

void init() {
    bet=5; betType=BET_RED; result=0;
    wheelAngle=0.0f; maxSpeed=0.5f; spinTicks=0; totalTicks=0;
    msg[0]='\0'; msgCol=C_DIM; rlState=BETTING;
    M5Cardputer.Speaker.setVolume(soundVol);
    redrawAll();
    snprintf(msg,sizeof(msg),"Place your bet and spin!"); msgCol=C_DIM; drawMsg();
}

bool tick() {
    if(M5Cardputer.Keyboard.isChange()&&M5Cardputer.Keyboard.isPressed()){
        auto st=M5Cardputer.Keyboard.keysState();
        bool act=st.enter;
        for(char c:st.word){
            if(c=='q'||c=='Q') return false;
            if(c=='m'||c=='M'){soundOn=!soundOn;}
            if(c==']'){soundVol=min(soundVol+50,250);M5Cardputer.Speaker.setVolume(soundVol);}
            if(c=='['){soundVol=max(soundVol-50,50);M5Cardputer.Speaker.setVolume(soundVol);}
            if(c==' ') act=true;
            if(rlState==BETTING||rlState==RESULT){
                if(c=='+'||c=='='){bet=min(bet+1,20);drawHUD();}
                if(c=='-')       {bet=max(bet-1,1); drawHUD();}
                if(c=='a'||c=='A'){betType=(betType-1+BET_NTYPES)%BET_NTYPES;drawHUD();}
                if(c=='d'||c=='D'){betType=(betType+1)%BET_NTYPES;drawHUD();}
            }
        }
        if(act){
            if(rlState==BETTING) startSpin();
            else if(rlState==RESULT){
                if(Persist::credits<=0){Persist::credits=100;Persist::save();bet=min(bet,5);}
                rlState=BETTING; msg[0]='\0';
                redrawAll();
                snprintf(msg,sizeof(msg),"Place your bet and spin!"); msgCol=C_DIM; drawMsg();
            }
        }
    }

    if(rlState==SPINNING){
        spinTicks++;
        float progress=(float)spinTicks/(float)totalTicks;
        float spd=maxSpeed*(1.0f-progress*0.96f);
        if(spd<0.012f) spd=0.012f;
        wheelAngle+=spd;

        if(spinTicks%2==0){
            sndTick();
            auto& d=M5Cardputer.Display;
            d.fillEllipse(WCX,WCY,WR_OA+2,WR_OB+2,C_BG);
            drawWheel(wheelAngle);

            // Anticipation display in last 30 ticks
            if(spinTicks >= totalTicks-30 && spinTicks < totalTicks){
                int potIdx=pocketUnderPointer();
                int potNum=WHEEL_ORDER[potIdx];
                bool potWin=betWins(betType,potNum);
                auto& d2=M5Cardputer.Display;
                d2.fillRect(0,112,240,10,C_BG);
                if(potWin){
                    int potPay=bet*BET_PAY[betType];
                    char pb[24]; snprintf(pb,sizeof(pb),">>> +%d <<<",potPay);
                    d2.setTextSize(1); d2.setTextColor(C_GOLD);
                    d2.setCursor((240-d2.textWidth(pb))/2,113); d2.print(pb);
                }
            }
        }

        if(spinTicks>=totalTicks){
            int bestIdx=pocketUnderPointer();
            result=WHEEL_ORDER[bestIdx];
            if(result==0) Persist::unlock(Persist::ACH_LUCKY_ZERO);

            auto& d=M5Cardputer.Display;
            d.fillEllipse(WCX,WCY,WR_OA+2,WR_OB+2,C_BG);
            drawWheel(wheelAngle, bestIdx);

            bool win=betWins(betType,result);
            int payout=win?bet*BET_PAY[betType]:0;
            Persist::credits+=payout;
            Persist::updateCredits();

            const char* col_name=result==0?"GREEN":(isRed(result)?"RED":"BLACK");
            if(win){
                snprintf(msg,sizeof(msg),"%d %s  WIN! +%d coins",result,col_name,payout);
                msgCol=C_GOLD; sndWin();
            } else {
                snprintf(msg,sizeof(msg),"%d %s  No win.",result,col_name);
                msgCol=(result==0)?C_ZERO:(isRed(result)?C_RED:C_DIM); sndLose();
            }
            rlState=RESULT;
            drawHUD(); drawMsg(); drawFooter();
            if(Persist::credits<=0){
                snprintf(msg,sizeof(msg),"BROKE! SPC=reset Q=menu");msgCol=C_RED;drawMsg();
                Persist::credits=100; Persist::save();
            }
        }
    }

    delay(30);
    return true;
}

} // namespace Roulette
