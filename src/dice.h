#pragma once
#include <M5Cardputer.h>

namespace Dice {

static const uint16_t C_BG   = 0x0000;
static const uint16_t C_TXT  = 0xFFFF;
static const uint16_t C_DIM  = 0x4208;
static const uint16_t C_GOLD = 0xFEA0;
static const uint16_t C_RED  = 0xF800;
static const uint16_t C_GRN  = 0x07E0;

static int  diceVals[5];
static bool locked[5];
static int  rollsLeft;
static int  credits, bet;
static char msg[48];
static uint16_t msgCol;
static bool soundOn = false;
static int  soundVol = 100;
static enum { BETTING, ROLLING, SCORED } dState;

static void sndRoll() {
    if (!soundOn) return;
    for (int i=0;i<4;i++) { M5Cardputer.Speaker.tone(400+random(800),25); delay(40); }
}
static void sndWin()  { if (!soundOn) return; M5Cardputer.Speaker.tone(880,80); delay(90); M5Cardputer.Speaker.tone(1100,120); }
static void sndLose() { if (!soundOn) return; M5Cardputer.Speaker.tone(300,200); }

// Returns payout multiplier and fills desc
static int scoreHand(char* desc) {
    int cnt[7]={};
    for (int i=0;i<5;i++) cnt[diceVals[i]]++;
    int sorted[5]; for (int i=0;i<5;i++) sorted[i]=diceVals[i];
    for (int i=0;i<4;i++) for (int j=i+1;j<5;j++) if (sorted[j]<sorted[i]) { int t=sorted[i];sorted[i]=sorted[j];sorted[j]=t; }
    bool straight=true;
    for (int i=0;i<4;i++) if (sorted[i+1]!=sorted[i]+1) { straight=false; break; }
    int maxCnt=0,pairs=0,trips=0;
    for (int v=1;v<=6;v++) {
        if (cnt[v]>maxCnt) maxCnt=cnt[v];
        if (cnt[v]==2) pairs++;
        if (cnt[v]==3) trips++;
    }
    bool fullHouse=(trips==1&&pairs==1);
    if (maxCnt==5) { strcpy(desc,"FIVE OF A KIND!");  return 50; }
    if (straight)  { strcpy(desc,"STRAIGHT!");        return 12; }
    if (maxCnt==4) { strcpy(desc,"FOUR OF A KIND");   return 15; }
    if (fullHouse) { strcpy(desc,"FULL HOUSE!");      return 8;  }
    if (maxCnt==3) { strcpy(desc,"THREE OF A KIND");  return 4;  }
    if (pairs==2)  { strcpy(desc,"TWO PAIRS");        return 3;  }
    if (pairs==1)  { strcpy(desc,"PAIR");             return 2;  }
    strcpy(desc,"No combo"); return 0;
}

static void rollUnlocked() {
    for (int i=0;i<5;i++) if (!locked[i]) diceVals[i]=1+random(6);
}

static void drawHeader() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,0,240,16,0x1800); d.setTextSize(1); d.setTextColor(C_GOLD);
    const char* t="<<  D I C E  R O L L  >>";
    d.setCursor((240-d.textWidth(t))/2,4); d.print(t);
}

static void drawHUD() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,17,240,11,C_BG); d.setTextSize(1);
    char l[20],r[24];
    snprintf(l,sizeof(l),"BET: %d",bet);
    snprintf(r,sizeof(r),"CREDITS: %d",credits);
    d.setTextColor(C_TXT);
    d.setCursor(4,18); d.print(l);
    d.setCursor(240-d.textWidth(r)-4,18); d.print(r);
}

static void drawDice() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,29,240,58,C_BG);
    // Each die: 38px wide box, spaced 46px apart, centered
    // 5*38 + 4*8 = 222, offset = (240-222)/2 = 9
    for (int i=0;i<5;i++) {
        int x=9+i*46, y=31;
        uint16_t col=(dState==ROLLING&&locked[i])?C_GOLD:C_TXT;
        if (dState==ROLLING&&!locked[i]) col=C_DIM;
        d.drawRect(x,y,38,28,col);
        d.setTextSize(2); d.setTextColor(col);
        char buf[4]; snprintf(buf,sizeof(buf),"%d",diceVals[i]);
        // center number in box (textSize2 = 12px wide)
        d.setCursor(x+13,y+6); d.print(buf);
        // lock indicator below
        d.setTextSize(1);
        if (dState==ROLLING) {
            d.setTextColor(locked[i]?C_GOLD:C_DIM);
            int tw=d.textWidth(locked[i]?"HOLD":"----");
            d.setCursor(x+(38-tw)/2,y+32); d.print(locked[i]?"HOLD":"----");
        }
    }
}

static void drawScore() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,88,240,20,C_BG); d.setTextSize(1);
    if (dState==ROLLING || dState==SCORED) {
        char desc[32]; int mult=scoreHand(desc);
        if (mult>0) {
            d.setTextColor(C_GOLD);
            char buf[48]; snprintf(buf,sizeof(buf),"%s  x%d bet = +%d",desc,mult,mult*bet);
            d.setCursor((240-d.textWidth(buf))/2,90); d.print(buf);
        } else {
            d.setTextColor(C_DIM);
            d.setCursor((240-d.textWidth("No combo"))/2,90); d.print("No combo");
        }
    }
}

static void drawMsg() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,110,240,13,C_BG);
    if (!msg[0]) return;
    d.setTextSize(1); d.setTextColor(msgCol);
    d.setCursor((240-d.textWidth(msg))/2,112); d.print(msg);
}

static void drawFooter() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,110,240,25,0x0821); d.setTextSize(1);
    d.setTextColor(0x39E7); d.setCursor(4,113);
    if (dState==BETTING) {
        d.print("+/-=BET  SPC=ROLL  Q=MENU");
    } else if (dState==ROLLING) {
        char buf[48]; snprintf(buf,sizeof(buf),"1-5=HOLD  SPC=ROLL(%d left)  Q=MENU",rollsLeft);
        d.print(buf);
    } else {
        d.print("SPC=AGAIN  Q=MENU");
    }
    d.setCursor(4,124); d.setTextColor(soundOn?C_GRN:C_DIM);
    if (soundOn) { char v[20]; snprintf(v,sizeof(v),"M=SFX:ON  []=VOL:%d",soundVol/50); d.print(v); }
    else d.print("M=SFX:OFF");
}

static void drawRollsLeft() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,73,240,14,C_BG); d.setTextSize(1); d.setTextColor(C_DIM);
    char buf[32]; snprintf(buf,sizeof(buf),"Rolls remaining: %d",rollsLeft);
    d.setCursor((240-d.textWidth(buf))/2,75); d.print(buf);
}

static void startRolling() {
    if (credits<bet) { snprintf(msg,sizeof(msg),"Not enough credits!"); msgCol=C_RED; drawMsg(); return; }
    credits-=bet;
    for (int i=0;i<5;i++) { locked[i]=false; diceVals[i]=1+random(6); }
    rollsLeft=2; // first roll already done, 2 more available
    dState=ROLLING;
    sndRoll();
    drawHUD(); drawDice(); drawRollsLeft(); drawScore(); drawFooter();
    snprintf(msg,sizeof(msg),""); drawMsg();
}

static void scoreRound() {
    char desc[32]; int mult=scoreHand(desc);
    int won=mult*bet;
    credits+=won;
    if (won>0) { snprintf(msg,sizeof(msg),"%s  +%d coins!",desc,won); msgCol=C_GOLD; sndWin(); }
    else       { snprintf(msg,sizeof(msg),"No combo. Better luck!"); msgCol=C_DIM; sndLose(); }
    dState=SCORED;
    drawHUD(); drawScore(); drawFooter();
    // Draw final dice without hold labels
    auto& d=M5Cardputer.Display;
    d.fillRect(0,29,240,58,C_BG);
    for (int i=0;i<5;i++) {
        int x=9+i*46, y=31;
        d.drawRect(x,y,38,28,C_TXT);
        d.setTextSize(2); d.setTextColor(C_TXT);
        char buf[4]; snprintf(buf,sizeof(buf),"%d",diceVals[i]);
        d.setCursor(x+13,y+6); d.print(buf);
    }
    d.fillRect(0,73,240,14,C_BG);
    if (credits<=0) {
        snprintf(msg,sizeof(msg),"BROKE! SPC=reset Q=menu"); msgCol=C_RED;
    }
    drawMsg();
}

void init() {
    credits=100; bet=5; msg[0]='\0'; msgCol=C_DIM; dState=BETTING;
    for (int i=0;i<5;i++) { diceVals[i]=i+1; locked[i]=false; }
    M5Cardputer.Speaker.setVolume(soundVol);
    auto& d=M5Cardputer.Display;
    d.fillScreen(C_BG);
    drawHeader(); drawHUD(); drawDice();
    d.fillRect(0,73,240,40,C_BG);
    snprintf(msg,sizeof(msg),"Set your bet and roll!"); msgCol=C_DIM;
    drawMsg(); drawFooter();
}

bool tick() {
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        auto st=M5Cardputer.Keyboard.keysState();
        bool wantAction=st.enter;
        for (char c : st.word) {
            if (c=='q'||c=='Q') return false;
            if (c=='m'||c=='M') { soundOn=!soundOn; drawFooter(); }
            if (c==']') { soundVol=min(soundVol+50,250); M5Cardputer.Speaker.setVolume(soundVol); drawFooter(); }
            if (c=='[') { soundVol=max(soundVol-50, 50); M5Cardputer.Speaker.setVolume(soundVol); drawFooter(); }

            if (dState==BETTING) {
                if (c==' ') wantAction=true;
                if (c=='+'||c=='=') { bet=min(bet+1,20); drawHUD(); }
                if (c=='-')        { bet=max(bet-1,1);  drawHUD(); }
            } else if (dState==ROLLING) {
                if (c==' ') wantAction=true;
                // Toggle locks with 1-5
                if (c>='1'&&c<='5') {
                    int idx=c-'1';
                    locked[idx]=!locked[idx];
                    drawDice();
                }
            } else if (dState==SCORED) {
                if (c==' ') wantAction=true;
            }
        }

        if (wantAction) {
            if (dState==BETTING) {
                startRolling();
            } else if (dState==ROLLING) {
                if (rollsLeft>0) {
                    rollsLeft--;
                    rollUnlocked();
                    sndRoll();
                    drawDice(); drawRollsLeft(); drawScore();
                    if (rollsLeft==0) scoreRound();
                } else {
                    scoreRound();
                }
            } else if (dState==SCORED) {
                if (credits<=0) { credits=100; bet=5; }
                dState=BETTING;
                for (int i=0;i<5;i++) { diceVals[i]=i+1; locked[i]=false; }
                msg[0]='\0';
                auto& d=M5Cardputer.Display;
                d.fillScreen(C_BG);
                drawHeader(); drawHUD(); drawDice();
                d.fillRect(0,73,240,40,C_BG);
                snprintf(msg,sizeof(msg),"Set your bet and roll!"); msgCol=C_DIM;
                drawMsg(); drawFooter();
            }
        }
    }
    delay(50);
    return true;
}

} // namespace Dice
