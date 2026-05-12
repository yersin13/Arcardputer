#pragma once
#include <M5Cardputer.h>

namespace Dice {

static const uint16_t C_BG       = 0x0000;
static const uint16_t C_TXT      = 0xFFFF;
static const uint16_t C_DIM      = 0x4208;
static const uint16_t C_GOLD     = 0xFEA0;
static const uint16_t C_RED      = 0xF800;
static const uint16_t C_GRN      = 0x07E0;
static const uint16_t C_DIE      = 0xFFFF;
static const uint16_t C_PIP      = 0x2104;
static const uint16_t C_HELD_BG  = 0x9CE0;
static const uint16_t C_HELD_BR  = 0xFEA0;

// ── Die geometry ──────────────────────────────────────────────────────────
static const int DIE_W  = 38;
static const int DIE_Y  = 28;
static const int DIE_X0 = 9;   // (240 - 5*38 - 4*8) / 2 = 9
static const int DIE_SP = 46;  // stride (38 + 8 gap)

static const int PL=9, PC=19, PR=29;  // pip x offsets
static const int PT=9, PM=19, PB=29;  // pip y offsets
static const int PRAD=4;

static void drawPips(int x, int y, int val, uint16_t col) {
    auto& d = M5Cardputer.Display;
    int L=x+PL, C=x+PC, R=x+PR, T=y+PT, M=y+PM, B=y+PB;
    switch (val) {
        case 1: d.fillCircle(C,M,PRAD,col); break;
        case 2: d.fillCircle(R,T,PRAD,col); d.fillCircle(L,B,PRAD,col); break;
        case 3: d.fillCircle(R,T,PRAD,col); d.fillCircle(C,M,PRAD,col); d.fillCircle(L,B,PRAD,col); break;
        case 4: d.fillCircle(L,T,PRAD,col); d.fillCircle(R,T,PRAD,col);
                d.fillCircle(L,B,PRAD,col); d.fillCircle(R,B,PRAD,col); break;
        case 5: d.fillCircle(L,T,PRAD,col); d.fillCircle(R,T,PRAD,col); d.fillCircle(C,M,PRAD,col);
                d.fillCircle(L,B,PRAD,col); d.fillCircle(R,B,PRAD,col); break;
        case 6: d.fillCircle(L,T,PRAD,col); d.fillCircle(L,M,PRAD,col); d.fillCircle(L,B,PRAD,col);
                d.fillCircle(R,T,PRAD,col); d.fillCircle(R,M,PRAD,col); d.fillCircle(R,B,PRAD,col); break;
    }
}

static void drawOneDie(int i, int val, bool held) {
    auto& d = M5Cardputer.Display;
    int x = DIE_X0 + i * DIE_SP, y = DIE_Y;
    uint16_t bg = held ? C_HELD_BG : C_DIE;
    uint16_t br = held ? C_HELD_BR : C_DIM;
    d.fillRoundRect(x, y, DIE_W, DIE_W, 4, bg);
    d.drawRoundRect(x, y, DIE_W, DIE_W, 4, br);
    if (held) d.drawRoundRect(x+1, y+1, DIE_W-2, DIE_W-2, 3, br);
    drawPips(x, y, val, C_PIP);
    // hold label below
    d.setTextSize(1);
    const char* lbl = held ? "HOLD" : "----";
    d.setTextColor(held ? C_GOLD : C_DIM);
    d.fillRect(x, y+DIE_W+2, DIE_W, 9, C_BG);
    d.setCursor(x + (DIE_W - d.textWidth(lbl)) / 2, y+DIE_W+2);
    d.print(lbl);
}

static void drawAllDice() {
    for (int i = 0; i < 5; i++) drawOneDie(i, /* filled by state */ 0, false); // placeholder
}

// ── State ─────────────────────────────────────────────────────────────────
static int  diceVals[5];
static bool locked[5];
static int  rollsLeft;
static int  credits, bet;
static char msg[48];
static uint16_t msgCol;
static bool soundOn  = false;
static int  soundVol = 100;
static enum { BETTING, ROLLING, SCORED } dState;

static void drawDice() {
    for (int i = 0; i < 5; i++) drawOneDie(i, diceVals[i], locked[i]);
}

// ── Sound ─────────────────────────────────────────────────────────────────
static void sndRoll() {
    if (!soundOn) return;
    for (int i=0;i<5;i++){ M5Cardputer.Speaker.tone(300+random(600),30); delay(45); }
}
static void sndWin()  { if (!soundOn) return; M5Cardputer.Speaker.tone(880,80); delay(90); M5Cardputer.Speaker.tone(1100,120); }
static void sndLose() { if (!soundOn) return; M5Cardputer.Speaker.tone(300,200); }

// ── Scoring ───────────────────────────────────────────────────────────────
static int scoreHand(char* desc) {
    int cnt[7]={};
    for (int i=0;i<5;i++) cnt[diceVals[i]]++;
    int s[5]; for(int i=0;i<5;i++) s[i]=diceVals[i];
    for(int i=0;i<4;i++) for(int j=i+1;j<5;j++) if(s[j]<s[i]){int t=s[i];s[i]=s[j];s[j]=t;}
    bool st=true;
    for(int i=0;i<4;i++) if(s[i+1]!=s[i]+1){st=false;break;}
    int mx=0,p=0,tr=0;
    for(int v=1;v<=6;v++){if(cnt[v]>mx)mx=cnt[v];if(cnt[v]==2)p++;if(cnt[v]==3)tr++;}
    if(mx==5){strcpy(desc,"FIVE OF A KIND!");return 50;}
    if(st)   {strcpy(desc,"STRAIGHT!");      return 12;}
    if(mx==4){strcpy(desc,"FOUR OF A KIND"); return 15;}
    if(tr==1&&p==1){strcpy(desc,"FULL HOUSE!");return 8;}
    if(mx==3){strcpy(desc,"THREE OF A KIND");return 4;}
    if(p==2) {strcpy(desc,"TWO PAIRS");      return 3;}
    if(p==1) {strcpy(desc,"PAIR");           return 2;}
    strcpy(desc,"No combo"); return 0;
}

// ── Draw ──────────────────────────────────────────────────────────────────
static void drawHeader() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0,0,240,16,0x1800); d.setTextSize(1); d.setTextColor(C_GOLD);
    const char* t = "<<  D I C E  R O L L  >>";
    d.setCursor((240-d.textWidth(t))/2,4); d.print(t);
}

static void drawHUD() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0,17,240,11,C_BG); d.setTextSize(1);
    char l[20],r[24];
    snprintf(l,sizeof(l),"BET: %d",bet);
    snprintf(r,sizeof(r),"CREDITS: %d",credits);
    d.setTextColor(C_TXT);
    d.setCursor(4,18); d.print(l);
    d.setCursor(240-d.textWidth(r)-4,18); d.print(r);
}

static void drawRollsLeft() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0,80,240,10,C_BG);
    if (dState!=ROLLING) return;
    d.setTextSize(1);
    uint16_t col = rollsLeft>1 ? C_TXT : (rollsLeft==1 ? C_GOLD : C_RED);
    d.setTextColor(col);
    char buf[32]; snprintf(buf,sizeof(buf),"Rolls remaining: %d",rollsLeft);
    d.setCursor((240-d.textWidth(buf))/2,80); d.print(buf);
}

static void drawScorePreview() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0,92,240,12,C_BG);
    if (dState==BETTING) return;
    d.setTextSize(1);
    char desc[32]; int mult=scoreHand(desc);
    if (mult>0) {
        d.setTextColor(C_GOLD);
        char buf[48]; snprintf(buf,sizeof(buf),"%s  x%d = +%d",desc,mult,mult*bet);
        d.setCursor((240-d.textWidth(buf))/2,92); d.print(buf);
    } else {
        d.setTextColor(C_DIM);
        const char* nc="No combo";
        d.setCursor((240-d.textWidth(nc))/2,92); d.print(nc);
    }
}

static void drawMsg() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0,106,240,13,C_BG);
    if (!msg[0]) return;
    d.setTextSize(1); d.setTextColor(msgCol);
    d.setCursor((240-d.textWidth(msg))/2,106); d.print(msg);
}

static void drawFooter() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0,120,240,15,0x0821); d.setTextSize(1);
    d.setTextColor(0x39E7); d.setCursor(4,122);
    if (dState==BETTING)      d.print("+/-=BET   SPC=ROLL   Q=MENU");
    else if (dState==ROLLING) { char b[48]; snprintf(b,sizeof(b),"1-5=HOLD  SPC=ROLL(%d left)  Q=MENU",rollsLeft); d.print(b); }
    else                      d.print("SPC=AGAIN   Q=MENU");
    // sound on second line if room
    d.setCursor(4,131); d.setTextColor(soundOn?C_GRN:C_DIM);
    if (soundOn){ char v[22]; snprintf(v,sizeof(v),"M=ON  []=VOL:%d/5",soundVol/50); d.print(v); }
    else d.print("M=SFX:OFF");
}

// ── Roll animation ────────────────────────────────────────────────────────
static void animateRoll() {
    for (int f=0;f<8;f++) {
        for (int i=0;i<5;i++) if (!locked[i]) diceVals[i]=1+random(6);
        drawDice();
        delay(50);
    }
}

static void rollUnlocked() {
    for (int i=0;i<5;i++) if (!locked[i]) diceVals[i]=1+random(6);
}

// ── Game actions ──────────────────────────────────────────────────────────
static void startRolling() {
    if (credits<bet){ snprintf(msg,sizeof(msg),"Not enough credits!"); msgCol=C_RED; drawMsg(); return; }
    credits -= bet;
    for (int i=0;i<5;i++){ locked[i]=false; diceVals[i]=1+random(6); }
    rollsLeft = 2;
    dState = ROLLING;
    animateRoll();
    rollUnlocked();
    drawDice(); drawHUD(); drawRollsLeft(); drawScorePreview(); drawFooter();
    msg[0]='\0'; drawMsg();
}

static void scoreRound() {
    char desc[32]; int mult=scoreHand(desc);
    int won = mult * bet;
    credits += won;
    if (won>0){ snprintf(msg,sizeof(msg),"%s  +%d coins!",desc,won); msgCol=C_GOLD; sndWin(); }
    else      { snprintf(msg,sizeof(msg),"No combo. Better luck!"); msgCol=C_DIM; sndLose(); }
    dState = SCORED;
    // show final dice without hold labels
    for (int i=0;i<5;i++) locked[i]=false;
    drawDice();
    auto& d = M5Cardputer.Display;
    d.fillRect(0,80,240,25,C_BG);  // clear rolls + score preview
    drawHUD(); drawScorePreview(); drawFooter();
    if (credits<=0){ snprintf(msg,sizeof(msg),"BROKE!  SPC=reset  Q=menu"); msgCol=C_RED; }
    drawMsg();
}

// ── Public ────────────────────────────────────────────────────────────────
void init() {
    credits=100; bet=5; msg[0]='\0'; msgCol=C_DIM; dState=BETTING;
    for (int i=0;i<5;i++){ diceVals[i]=i+1; locked[i]=false; }
    M5Cardputer.Speaker.setVolume(soundVol);
    auto& d = M5Cardputer.Display;
    d.fillScreen(C_BG);
    drawHeader(); drawHUD(); drawDice();
    d.fillRect(0,80,240,40,C_BG);
    snprintf(msg,sizeof(msg),"Set your bet and roll!"); msgCol=C_DIM;
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
            if (dState==BETTING) {
                if (c=='+'||c=='=') { bet=min(bet+1,20); drawHUD(); }
                if (c=='-')         { bet=max(bet-1,1);  drawHUD(); }
            } else if (dState==ROLLING) {
                if (c>='1'&&c<='5') {
                    int idx=c-'1';
                    locked[idx]=!locked[idx];
                    drawOneDie(idx,diceVals[idx],locked[idx]);
                }
            }
        }
        if (act) {
            if (dState==BETTING) {
                startRolling();
            } else if (dState==ROLLING) {
                if (rollsLeft>0) {
                    rollsLeft--;
                    animateRoll();
                    rollUnlocked();
                    drawDice(); drawRollsLeft(); drawScorePreview();
                    if (rollsLeft==0) { delay(400); scoreRound(); }
                    else              drawFooter();
                }
            } else if (dState==SCORED) {
                if (credits<=0){ credits=100; bet=5; }
                dState=BETTING;
                for (int i=0;i<5;i++){ diceVals[i]=i+1; locked[i]=false; }
                msg[0]='\0';
                M5Cardputer.Display.fillScreen(C_BG);
                drawHeader(); drawHUD(); drawDice();
                M5Cardputer.Display.fillRect(0,80,240,40,C_BG);
                snprintf(msg,sizeof(msg),"Set your bet and roll!"); msgCol=C_DIM;
                drawMsg(); drawFooter();
            }
        }
    }
    delay(50);
    return true;
}

} // namespace Dice
