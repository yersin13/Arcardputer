#pragma once
#include <M5Cardputer.h>
#include <math.h>

namespace Roulette {

static const uint16_t C_BG   = 0x0000;
static const uint16_t C_TXT  = 0xFFFF;
static const uint16_t C_DIM  = 0x4208;
static const uint16_t C_GOLD = 0xFEA0;
static const uint16_t C_RED  = 0xF800;
static const uint16_t C_GRN  = 0x07E0;
static const uint16_t C_BLK  = 0x2104;
static const uint16_t C_MGN  = 0xF81F;
static const uint16_t C_CYN  = 0x07FF;
static const uint16_t C_ZERO = 0x03E0; // green for 0

// European roulette pocket order (0 at index 0, then clockwise)
static const int WHEEL_ORDER[37] = {
    0,32,15,19,4,21,2,25,17,34,6,27,13,36,11,30,8,23,10,5,24,16,33,1,20,14,31,9,22,18,29,7,28,12,35,3,26
};
// Red numbers in European roulette
static const int RED_NUMS[18] = {1,3,5,7,9,12,14,16,18,19,21,23,25,27,30,32,34,36};

static bool isRed(int n) {
    if(n==0) return false;
    for(int i=0;i<18;i++) if(RED_NUMS[i]==n) return true;
    return false;
}

// ── Wheel geometry ────────────────────────────────────────────────────────
static const int WCX = 120, WCY = 68;  // wheel center
static const int WR_OUT = 58;           // outer radius
static const int WR_IN  = 30;           // inner radius (ball track)
static const int WR_NUM = 44;           // number label radius

static const float TWO_PI_F = 6.2831853f;
static const float SEG = TWO_PI_F / 37.0f; // angle per segment

static void drawWheel(float angleOffset, int highlighted=-1) {
    auto& d = M5Cardputer.Display;
    // Draw 37 segments
    for(int i=0;i<37;i++){
        float a0 = angleOffset + i*SEG;
        float a1 = a0 + SEG;
        float amid = a0 + SEG*0.5f;
        int n = WHEEL_ORDER[i];
        uint16_t col = (n==0) ? C_ZERO : (isRed(n)?C_RED:C_BLK);
        if(highlighted==i) col = C_GOLD;
        // Fill segment with multiple triangles for smoother look
        int steps=4;
        for(int s=0;s<steps;s++){
            float sa=a0+s*(a1-a0)/steps;
            float sb=a0+(s+1)*(a1-a0)/steps;
            int x0=WCX+(int)(cosf(sa)*WR_IN);  int y0=WCY+(int)(sinf(sa)*WR_IN);
            int x1=WCX+(int)(cosf(sb)*WR_IN);  int y1=WCY+(int)(sinf(sb)*WR_IN);
            int x2=WCX+(int)(cosf(sa)*WR_OUT); int y2=WCY+(int)(sinf(sa)*WR_OUT);
            int x3=WCX+(int)(cosf(sb)*WR_OUT); int y3=WCY+(int)(sinf(sb)*WR_OUT);
            d.fillTriangle(x0,y0,x1,y1,x2,y2,col);
            d.fillTriangle(x1,y1,x2,y2,x3,y3,col);
        }
        // Separator lines
        int lx0=WCX+(int)(cosf(a0)*WR_IN);  int ly0=WCY+(int)(sinf(a0)*WR_IN);
        int lx1=WCX+(int)(cosf(a0)*WR_OUT); int ly1=WCY+(int)(sinf(a0)*WR_OUT);
        d.drawLine(lx0,ly0,lx1,ly1,C_GOLD);
    }
    // Inner circle (center hub)
    d.fillCircle(WCX,WCY,WR_IN-1,0x0821);
    d.drawCircle(WCX,WCY,WR_IN,C_GOLD);
    d.drawCircle(WCX,WCY,WR_OUT,C_GOLD);
    // Draw numbers in segments
    d.setTextSize(1);
    for(int i=0;i<37;i++){
        float amid=angleOffset+i*SEG+SEG*0.5f;
        int nx=WCX+(int)(cosf(amid)*WR_NUM);
        int ny=WCY+(int)(sinf(amid)*WR_NUM);
        int n=WHEEL_ORDER[i];
        char buf[4]; snprintf(buf,sizeof(buf),"%d",n);
        uint16_t tc=(n==0)?C_BG:(isRed(n)?C_TXT:C_TXT);
        d.setTextColor(tc);
        d.setCursor(nx-d.textWidth(buf)/2, ny-4);
        d.print(buf);
    }
    // Pointer (top = -PI/2)
    int px=WCX+(int)(cosf(angleOffset-TWO_PI_F/4)*WR_OUT)-4;
    // Fixed pointer at top
    d.fillTriangle(WCX,WCY-WR_OUT-2, WCX-4,WCY-WR_OUT+4, WCX+4,WCY-WR_OUT+4, C_GOLD);
}

// ── Bet types ─────────────────────────────────────────────────────────────
enum BetType { BET_RED=0, BET_BLACK, BET_ODD, BET_EVEN, BET_LOW, BET_HIGH, BET_DOZEN1, BET_DOZEN2, BET_DOZEN3, BET_NUM_TYPES };
static const char* BET_NAMES[BET_NUM_TYPES] = {"RED","BLACK","ODD","EVEN","1-18","19-36","1-12","13-24","25-36"};
static const int   BET_PAY [BET_NUM_TYPES]  = {  2,     2,    2,    2,    2,     2,      3,      3,      3    };

static bool betWins(int betIdx, int n) {
    if(n==0) return false;
    switch(betIdx){
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
static int   credits, bet;
static int   betType;
static int   result;          // winning number
static float wheelAngle;      // current visual angle offset
static float spinSpeed;       // radians per tick
static int   spinTicks;
static int   totalTicks;
static char  msg[56]; static uint16_t msgCol;
static bool  soundOn=false; static int soundVol=100;
static enum  { BETTING, SPINNING, RESULT } rlState;

static void sndTick() { if(!soundOn)return; M5Cardputer.Speaker.tone(800+(int)(spinSpeed*200),15); }
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
    d.fillRect(0,17,240,11,C_BG); d.setTextSize(1);
    char l[28],r[24];
    snprintf(l,sizeof(l),"BET:%d on %s",bet,BET_NAMES[betType]);
    snprintf(r,sizeof(r),"CREDITS:%d",credits);
    uint16_t lc=(betType==BET_RED)?C_RED:(betType==BET_BLACK?C_DIM:C_TXT);
    d.setTextColor(lc); d.setCursor(4,18); d.print(l);
    d.setTextColor(C_TXT); d.setCursor(240-d.textWidth(r)-4,18); d.print(r);
}
static void drawBetSelector() {
    auto& d=M5Cardputer.Display;
    // Two rows of bet buttons at the bottom of the wheel area, inside WCY+WR_OUT
    // Place below wheel: WCY+WR_OUT ~ 126 but screen is 135. Use compact row.
    d.fillRect(0,WCY+WR_OUT+2,240,6,C_BG);
    // just show prev/next hint, current bet highlighted
    d.setTextSize(1); d.setTextColor(0x39E7);
    const char* hint="A/D=BetType  +/-=BetAmt  SPC=SPIN  Q=MENU";
    d.setCursor((240-d.textWidth(hint))/2, WCY+WR_OUT+2);
    d.print(hint);
}
static void drawMsg() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,126,240,9,C_BG);
    if(!msg[0]) return;
    d.setTextSize(1); d.setTextColor(msgCol);
    d.setCursor((240-d.textWidth(msg))/2,126); d.print(msg);
}

static void redrawAll() {
    M5Cardputer.Display.fillScreen(C_BG);
    drawHeader(); drawHUD(); drawWheel(wheelAngle); drawBetSelector(); drawMsg();
}

static void startSpin() {
    if(credits<bet){snprintf(msg,sizeof(msg),"Not enough credits!");msgCol=C_RED;drawMsg();return;}
    credits-=bet;
    // Pick result: random pocket
    int pocketIdx=random(37);
    result=WHEEL_ORDER[pocketIdx];
    // Calculate target angle so that pocket lands at the pointer (top = -PI/2)
    // Pointer is at angle -PI/2 in screen coords. Segment i is at angleOffset + i*SEG + SEG/2 = -PI/2
    // So angleOffset = -PI/2 - pocketIdx*SEG - SEG/2
    float targetAngle = -TWO_PI_F/4.0f - pocketIdx*SEG - SEG*0.5f;
    // Add random full rotations (5-10)
    int extraSpins = 5 + random(6);
    float totalAngle = wheelAngle + extraSpins*TWO_PI_F + fmod(targetAngle - fmod(wheelAngle, TWO_PI_F) + TWO_PI_F*2, TWO_PI_F);
    totalTicks = 60 + extraSpins*8;
    spinSpeed = (totalAngle - wheelAngle) / totalTicks; // approximate; we'll use easing
    spinTicks = 0; rlState = SPINNING;
    msg[0]='\0'; drawHUD(); drawMsg();
}

void init() {
    credits=100; bet=5; betType=BET_RED; result=0;
    wheelAngle=0.0f; spinSpeed=0.0f; spinTicks=0; totalTicks=0;
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
            if(rlState==BETTING){
                if(c=='+'||c=='='){bet=min(bet+1,20);drawHUD();}
                if(c=='-')       {bet=max(bet-1,1); drawHUD();}
                if(c=='a'||c=='A'){betType=(betType-1+BET_NUM_TYPES)%BET_NUM_TYPES;drawHUD();}
                if(c=='d'||c=='D'){betType=(betType+1)%BET_NUM_TYPES;drawHUD();}
            } else if(rlState==RESULT){
                // handled by act below
            }
        }
        if(act){
            if(rlState==BETTING) startSpin();
            else if(rlState==RESULT){
                if(credits<=0){credits=100;bet=min(bet,5);}
                rlState=BETTING; msg[0]='\0';
                redrawAll();
                snprintf(msg,sizeof(msg),"Place your bet and spin!"); msgCol=C_DIM; drawMsg();
            }
        }
    }

    if(rlState==SPINNING){
        spinTicks++;
        float progress=(float)spinTicks/(float)totalTicks;
        // Ease-out: start fast, decelerate
        float easedSpeed = spinSpeed * (1.0f - progress*progress*0.85f) + 0.005f;
        if(easedSpeed<0.008f) easedSpeed=0.008f;
        wheelAngle += easedSpeed;
        // Redraw wheel every 2 ticks for smooth animation
        if(spinTicks%2==0){
            // Only redraw wheel area
            auto& d=M5Cardputer.Display;
            d.fillCircle(WCX,WCY,WR_OUT+2,C_BG);
            drawWheel(wheelAngle);
            sndTick();
        }
        if(spinTicks>=totalTicks){
            // Snap to final position: find which pocket is at pointer
            // pointer at -PI/2. Find pocket whose center is closest to -PI/2 - wheelAngle (mod SEG)
            float ptrAngle = -TWO_PI_F/4.0f;
            int bestIdx=0; float bestDiff=999.0f;
            for(int i=0;i<37;i++){
                float segAngle = wheelAngle + i*SEG + SEG*0.5f;
                // Normalize to [0, 2PI)
                float diff = fmod(segAngle - ptrAngle, TWO_PI_F);
                if(diff<0) diff+=TWO_PI_F;
                if(diff>TWO_PI_F) diff-=TWO_PI_F;
                if(diff>TWO_PI_F/2) diff=TWO_PI_F-diff;
                if(diff<bestDiff){bestDiff=diff;bestIdx=i;}
            }
            result=WHEEL_ORDER[bestIdx];
            // Redraw final wheel with highlight
            auto& d=M5Cardputer.Display;
            d.fillCircle(WCX,WCY,WR_OUT+2,C_BG);
            drawWheel(wheelAngle, bestIdx);

            bool win=betWins(betType,result);
            int payout = win ? bet*BET_PAY[betType] : 0;
            credits+=payout;
            uint16_t nc=(result==0)?C_ZERO:(isRed(result)?C_RED:C_TXT);
            if(win){
                snprintf(msg,sizeof(msg),"  %d  %s  WIN! +%d coins",result,isRed(result)?"RED":"BLACK",payout);
                msgCol=C_GOLD; sndWin();
            } else {
                snprintf(msg,sizeof(msg),"  %d  %s  No win.",result,result==0?"GREEN":(isRed(result)?"RED":"BLACK"));
                msgCol=nc; sndLose();
            }
            rlState=RESULT;
            drawHUD(); drawMsg();
            if(credits<=0){snprintf(msg,sizeof(msg),"BROKE! SPC=reset Q=menu");msgCol=C_RED;drawMsg();}
            // Footer hint
            auto& d2=M5Cardputer.Display;
            d2.fillRect(0,WCY+WR_OUT+2,240,6,C_BG);
            d2.setTextSize(1); d2.setTextColor(0x39E7);
            const char* h="SPC=AGAIN  A/D=BetType  Q=MENU";
            d2.setCursor((240-d2.textWidth(h))/2,WCY+WR_OUT+2); d2.print(h);
        }
    }

    delay(30); // faster for smooth wheel spin
    return true;
}

} // namespace Roulette
