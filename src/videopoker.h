#pragma once
#include <M5Cardputer.h>

namespace VideoPoker {

static const uint16_t C_BG   = 0x0000;
static const uint16_t C_TXT  = 0xFFFF;
static const uint16_t C_DIM  = 0x4208;
static const uint16_t C_GOLD = 0xFEA0;
static const uint16_t C_RED  = 0xF800;
static const uint16_t C_GRN  = 0x07E0;
static const uint16_t C_CYN  = 0x07FF;
static const uint16_t C_CARD = 0xFFFF;
static const uint16_t C_BACK = 0x000F;

// ── Card geometry (5 cards: 5*38+4*6=214px, start x=13) ──────────────────
static const int CW=38, CH=52, CG=6, CX0=13;
static const int CARD_Y = 30;

struct Card { int val; int suit; };
static const uint16_t SUIT_COL[4] = { 0x2104, C_RED, C_RED, 0x2104 };
static const char* SUITS[4] = {"S","H","D","C"};

static void drawSuit(int cx, int cy, int suit, uint16_t col) {
    auto& d = M5Cardputer.Display;
    switch (suit) {
        case 0: d.fillCircle(cx-4,cy-2,4,col); d.fillCircle(cx+4,cy-2,4,col);
                d.fillTriangle(cx-7,cy+1,cx+7,cy+1,cx,cy+8,col);
                d.fillRect(cx-2,cy+6,4,4,col); break;
        case 1: d.fillCircle(cx-4,cy-2,4,col); d.fillCircle(cx+4,cy-2,4,col);
                d.fillTriangle(cx-7,cy+1,cx+7,cy+1,cx,cy+9,col); break;
        case 2: d.fillTriangle(cx,cy-7,cx+7,cy,cx-7,cy,col);
                d.fillTriangle(cx-7,cy,cx+7,cy,cx,cy+7,col); break;
        case 3: d.fillCircle(cx,cy-4,4,col); d.fillCircle(cx-5,cy+2,4,col);
                d.fillCircle(cx+5,cy+2,4,col); d.fillRect(cx-2,cy+5,4,5,col); break;
    }
}

static void drawCard(int i, Card c, bool held, bool hidden=false) {
    auto& d = M5Cardputer.Display;
    int x = CX0 + i*(CW+CG), y = CARD_Y;
    if (hidden) {
        d.fillRoundRect(x,y,CW,CH,4,C_BACK);
        d.drawRoundRect(x,y,CW,CH,4,C_DIM);
        for(int py=y+4;py<y+CH-3;py+=5) d.drawFastHLine(x+3,py,CW-6,0x0018);
        return;
    }
    uint16_t sc = SUIT_COL[c.suit];
    uint16_t border = held ? C_GOLD : sc;
    d.fillRoundRect(x,y,CW,CH,4,C_CARD);
    d.drawRoundRect(x,y,CW,CH,4,border);
    if (held) d.drawRoundRect(x+1,y+1,CW-2,CH-2,3,border);
    // Value
    d.setTextSize(1); d.setTextColor(sc);
    char val[4];
    if(c.val==1) strcpy(val,"A"); else if(c.val==11) strcpy(val,"J");
    else if(c.val==12) strcpy(val,"Q"); else if(c.val==13) strcpy(val,"K");
    else snprintf(val,sizeof(val),"%d",c.val);
    d.setCursor(x+3,y+3); d.print(val);
    // Suit symbol centered
    drawSuit(x+CW/2, y+CH/2+4, c.suit, sc);
    // HOLD label below
    d.fillRect(x,y+CH+2,CW,9,C_BG);
    d.setTextSize(1);
    d.setTextColor(held?C_GOLD:C_DIM);
    const char* lbl=held?"HOLD":"----";
    d.setCursor(x+(CW-d.textWidth(lbl))/2, y+CH+2); d.print(lbl);
}

// ── Hand evaluation ───────────────────────────────────────────────────────
static const char* HAND_NAME[] = {
    "High Card","Jacks or Better","Two Pair","Three of a Kind",
    "Straight","Flush","Full House","Four of a Kind","Straight Flush","ROYAL FLUSH!"
};
static const int   HAND_PAY[]  = { 0, 1, 2, 3, 4, 6, 9, 25, 50, 250 };

static int evalHand(Card h[5]) {
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
        if(cnt[vv]==4)quads++; if(cnt[vv]==3)trips++;
        if(cnt[vv]==2)pairs++; if(cnt[vv]>mx)mx=cnt[vv];
    }
    if(flush&&royal)       return 9;
    if(flush&&straight)    return 8;
    if(quads)              return 7;
    if(trips&&pairs)       return 6;
    if(flush)              return 5;
    if(straight)           return 4;
    if(trips)              return 3;
    if(pairs==2)           return 2;
    if(pairs==1){ for(int vv=1;vv<=13;vv++) if(cnt[vv]==2&&(vv==1||vv>=11)) return 1; }
    return 0;
}

// ── State ─────────────────────────────────────────────────────────────────
static Card  deck[52], hand[5];
static int   deckTop;
static bool  held[5];
static int   credits, bet;
static char  msg[48]; static uint16_t msgCol;
static bool  soundOn=false; static int soundVol=100;
static enum  { BETTING, DEALT, RESULT } vpState;

static void sndWin()  { if(!soundOn)return; M5Cardputer.Speaker.tone(880,80);delay(90);M5Cardputer.Speaker.tone(1100,120); }
static void sndLose() { if(!soundOn)return; M5Cardputer.Speaker.tone(300,200); }
static void sndCard() { if(!soundOn)return; M5Cardputer.Speaker.tone(1000,20); }

static void shuffle() {
    int k=0;
    for(int s=0;s<4;s++) for(int v=1;v<=13;v++) deck[k++]={v,s};
    for(int i=51;i>0;i--){int j=random(i+1);Card t=deck[i];deck[i]=deck[j];deck[j]=t;}
    deckTop=0;
}
static Card drawDeck() { if(deckTop>=52)shuffle(); return deck[deckTop++]; }

static void drawHeader() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,0,240,16,0x1800); d.setTextSize(1); d.setTextColor(C_GOLD);
    const char* t="<<  V I D E O  P O K E R  >>";
    d.setCursor((240-d.textWidth(t))/2,4); d.print(t);
}
static void drawHUD() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,17,240,11,C_BG); d.setTextSize(1);
    char l[20],r[24];
    snprintf(l,sizeof(l),"BET: %d",bet); snprintf(r,sizeof(r),"CREDITS: %d",credits);
    d.setTextColor(C_TXT); d.setCursor(4,18); d.print(l);
    d.setCursor(240-d.textWidth(r)-4,18); d.print(r);
}
static void drawPayTable() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,95,240,14,C_BG); d.setTextSize(1); d.setCursor(2,96);
    const char* shorts[]={"JB","2P","3K","ST","FL","FH","4K","SF","RF"};
    const int   pays[]  ={  1,  2,  3,  4,  6,  9, 25, 50,250};
    for(int i=0;i<9;i++){
        d.setTextColor(i>=7?C_GOLD:(i>=5?C_CYN:C_DIM));
        d.print(shorts[i]);
        d.setTextColor(C_DIM);
        char b[6]; snprintf(b,sizeof(b),"=%d ",pays[i]); d.print(b);
    }
}
static void drawMsg() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,111,240,13,C_BG);
    if(!msg[0])return;
    d.setTextSize(1); d.setTextColor(msgCol);
    d.setCursor((240-d.textWidth(msg))/2,111); d.print(msg);
}
static void drawFooter() {
    auto& d=M5Cardputer.Display;
    d.fillRect(0,125,240,10,0x0821); d.setTextSize(1); d.setTextColor(0x39E7);
    d.setCursor(2,126);
    if(vpState==BETTING)  d.print("+/-=BET  SPC=DEAL  Q=MENU");
    else if(vpState==DEALT) d.print("1-5=HOLD  SPC=DRAW  Q=MENU");
    else                   d.print("SPC=AGAIN  Q=MENU");
}

static void dealHand() {
    if(credits<bet){snprintf(msg,sizeof(msg),"Not enough credits!");msgCol=C_RED;drawMsg();return;}
    credits-=bet;
    for(int i=0;i<5;i++){held[i]=false;}
    // Animate: deal one card at a time
    M5Cardputer.Display.fillRect(0,CARD_Y,240,CH+12,C_BG);
    for(int i=0;i<5;i++){
        hand[i]=drawDeck(); sndCard();
        // Flash back then reveal
        drawCard(i,hand[i],false,true); delay(80);
        drawCard(i,hand[i],false,false); delay(80);
    }
    vpState=DEALT; msg[0]='\0';
    drawHUD(); drawMsg(); drawFooter();
}

static void drawHand() {
    M5Cardputer.Display.fillRect(0,CARD_Y,240,CH+12,C_BG);
    for(int i=0;i<5;i++) drawCard(i,hand[i],held[i]);
}

static void redraw() {
    for(int i=0;i<5;i++){
        if(!held[i]){
            hand[i]=drawDeck(); sndCard();
            drawCard(i,hand[i],false,true); delay(70);
            drawCard(i,hand[i],false,false); delay(70);
        }
    }
    // Evaluate
    int rank=evalHand(hand);
    int win=HAND_PAY[rank]*bet;
    credits+=win;
    if(win>0){
        snprintf(msg,sizeof(msg),"%s  +%d coins!",HAND_NAME[rank],win);
        msgCol=(rank>=7?C_GOLD:(rank>=4?C_CYN:C_GRN)); sndWin();
        // Flash winning cards
        for(int f=0;f<4;f++){
            for(int i=0;i<5;i++){
                auto& d=M5Cardputer.Display;
                d.drawRoundRect(CX0+i*(CW+CG),CARD_Y,CW,CH,4,f%2==0?C_GOLD:SUIT_COL[hand[i].suit]);
            }
            delay(120);
        }
    } else {
        snprintf(msg,sizeof(msg),"No win. %s",HAND_NAME[rank]); msgCol=C_DIM; sndLose();
    }
    vpState=RESULT;
    drawHUD(); drawMsg(); drawFooter();
    if(credits<=0){snprintf(msg,sizeof(msg),"BROKE!  SPC=reset  Q=menu");msgCol=C_RED;drawMsg();}
}

void init() {
    credits=100; bet=5; msg[0]='\0'; msgCol=C_DIM; vpState=BETTING;
    shuffle();
    M5Cardputer.Speaker.setVolume(soundVol);
    auto& d=M5Cardputer.Display;
    d.fillScreen(C_BG); drawHeader(); drawHUD();
    // Show face-down cards as preview
    for(int i=0;i<5;i++) drawCard(i,{1,0},false,true);
    drawPayTable();
    snprintf(msg,sizeof(msg),"Set your bet and deal!"); msgCol=C_DIM;
    drawMsg(); drawFooter();
}

bool tick() {
    if(M5Cardputer.Keyboard.isChange()&&M5Cardputer.Keyboard.isPressed()){
        auto st=M5Cardputer.Keyboard.keysState();
        bool act=st.enter;
        for(char c:st.word){
            if(c=='q'||c=='Q') return false;
            if(c=='m'||c=='M'){soundOn=!soundOn;drawFooter();}
            if(c==']'){soundVol=min(soundVol+50,250);M5Cardputer.Speaker.setVolume(soundVol);drawFooter();}
            if(c=='['){soundVol=max(soundVol-50,50);M5Cardputer.Speaker.setVolume(soundVol);drawFooter();}
            if(c==' ') act=true;
            if(vpState==BETTING){
                if(c=='+'||c=='='){bet=min(bet+1,20);drawHUD();}
                if(c=='-')       {bet=max(bet-1,1); drawHUD();}
            } else if(vpState==DEALT){
                if(c>='1'&&c<='5'){int i=c-'1';held[i]=!held[i];drawCard(i,hand[i],held[i]);}
            }
        }
        if(act){
            if(vpState==BETTING)      dealHand();
            else if(vpState==DEALT)   redraw();
            else if(vpState==RESULT){
                if(credits<=0){credits=100;bet=min(bet,5);}
                vpState=BETTING; msg[0]='\0';
                auto& d=M5Cardputer.Display;
                d.fillScreen(C_BG); drawHeader(); drawHUD();
                for(int i=0;i<5;i++) drawCard(i,{1,0},false,true);
                drawPayTable(); snprintf(msg,sizeof(msg),"Set your bet and deal!"); msgCol=C_DIM;
                drawMsg(); drawFooter();
            }
        }
    }
    delay(50);
    return true;
}

} // namespace VideoPoker
