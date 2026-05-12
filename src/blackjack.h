#pragma once
#include <M5Cardputer.h>

namespace Blackjack {

static const uint16_t C_BG   = 0x0000;
static const uint16_t C_TXT  = 0xFFFF;
static const uint16_t C_DIM  = 0x4208;
static const uint16_t C_GOLD = 0xFEA0;
static const uint16_t C_RED  = 0xF800;
static const uint16_t C_GRN  = 0x07E0;

// Card: val 1-13 (1=Ace,11=J,12=Q,13=K), suit 0-3 (S H D C)
struct Card { int val; int suit; };
static const char*     SUITS[4]    = { "S","H","D","C" };
static const uint16_t  SUIT_COL[4] = { C_TXT, C_RED, C_RED, C_TXT };

static Card  deck[52];
static int   deckTop;
static Card  pHand[11], dHand[11];
static int   pCount, dCount;
static int   credits, bet, origBet;
static char  msg[48];
static uint16_t msgCol;
static bool  soundOn = false;
static int   soundVol = 100;
static enum { BETTING, PLAYER_TURN, DEALER_TURN, RESULT } bjState;

static void sndCard() { if (!soundOn) return; M5Cardputer.Speaker.tone(1000,20); }
static void sndWin()  { if (!soundOn) return; M5Cardputer.Speaker.tone(880,80); delay(90); M5Cardputer.Speaker.tone(1100,120); }
static void sndLose() { if (!soundOn) return; M5Cardputer.Speaker.tone(300,200); }

static void shuffleDeck() {
    int k=0;
    for (int s=0;s<4;s++) for (int v=1;v<=13;v++) deck[k++]={v,s};
    for (int i=51;i>0;i--) { int j=random(i+1); Card t=deck[i]; deck[i]=deck[j]; deck[j]=t; }
    deckTop=0;
}

static Card dealCard() {
    if (deckTop>=52) shuffleDeck();
    return deck[deckTop++];
}

static int handTotal(Card* h, int n) {
    int tot=0, aces=0;
    for (int i=0;i<n;i++) {
        int v=h[i].val;
        if (v==1) { aces++; tot+=11; }
        else tot+=min(v,10);
    }
    while (tot>21&&aces>0) { tot-=10; aces--; }
    return tot;
}

static void printCard(Card c, bool hidden=false) {
    auto& d = M5Cardputer.Display;
    if (hidden) { d.setTextColor(C_DIM); d.print("[??]"); return; }
    d.setTextColor(SUIT_COL[c.suit]);
    char buf[8];
    if      (c.val==1)  snprintf(buf,sizeof(buf),"[A%s]", SUITS[c.suit]);
    else if (c.val==11) snprintf(buf,sizeof(buf),"[J%s]", SUITS[c.suit]);
    else if (c.val==12) snprintf(buf,sizeof(buf),"[Q%s]", SUITS[c.suit]);
    else if (c.val==13) snprintf(buf,sizeof(buf),"[K%s]", SUITS[c.suit]);
    else                snprintf(buf,sizeof(buf),"[%d%s]",c.val,SUITS[c.suit]);
    d.print(buf);
}

static void drawHeader() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0,0,240,16,0x1800);
    d.setTextSize(1); d.setTextColor(C_GOLD);
    const char* t = "<<  B L A C K J A C K  >>";
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

static void drawHands(bool hideDealer=true) {
    auto& d = M5Cardputer.Display;
    d.fillRect(0,29,240,66,C_BG); d.setTextSize(1);
    // Dealer
    d.setTextColor(C_DIM); d.setCursor(4,34); d.print("DEALER ");
    for (int i=0;i<dCount;i++) {
        printCard(dHand[i], hideDealer&&i==0);
        d.setTextColor(C_DIM); d.print(" ");
    }
    if (!hideDealer) {
        int t=handTotal(dHand,dCount);
        d.setTextColor(t>21?C_RED:C_TXT);
        char b[8]; snprintf(b,sizeof(b),"=%d",t); d.print(b);
    }
    d.drawFastHLine(0,52,240,0x2945);
    // Player
    d.setTextColor(C_DIM); d.setCursor(4,58); d.print("YOU    ");
    for (int i=0;i<pCount;i++) { printCard(pHand[i]); d.setTextColor(C_DIM); d.print(" "); }
    int pt=handTotal(pHand,pCount);
    d.setTextColor(pt>21?C_RED:C_TXT);
    char b[8]; snprintf(b,sizeof(b),"=%d",pt); d.print(b);
    d.drawFastHLine(0,76,240,0x2945);
}

static void drawMsg() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0,79,240,13,C_BG);
    if (!msg[0]) return;
    d.setTextSize(1); d.setTextColor(msgCol);
    d.setCursor((240-d.textWidth(msg))/2,81); d.print(msg);
}

static void drawFooter() {
    auto& d = M5Cardputer.Display;
    d.fillRect(0,94,240,41,0x0821); d.setTextSize(1);
    d.setTextColor(0x39E7);
    d.setCursor(4,97);
    if      (bjState==BETTING)     d.print("+/-=BET  SPC=DEAL  Q=MENU");
    else if (bjState==PLAYER_TURN) d.print("H=HIT  S=STAND  D=DOUBLE  Q=MENU");
    else                           d.print("SPC/Enter=NEXT  Q=MENU");
    d.setCursor(4,109);
    d.setTextColor(C_DIM); d.print("BJ=1.5x  WIN=2x  PUSH=ret  PAIR=4x");
    d.setCursor(4,121);
    d.setTextColor(soundOn?C_GRN:C_DIM);
    if (soundOn) { char v[20]; snprintf(v,sizeof(v),"M=SFX:ON  []=VOL:%d",soundVol/50); d.print(v); }
    else d.print("M=SFX:OFF");
}

static void evalHand() {
    int p=handTotal(pHand,pCount);
    int dv=handTotal(dHand,dCount);
    bool pBJ=(pCount==2&&p==21);
    bool dBJ=(dCount==2&&dv==21);
    if (p>21) {
        snprintf(msg,sizeof(msg),"Bust! You lose."); msgCol=C_DIM; sndLose();
    } else if (pBJ&&!dBJ) {
        int win=bet+bet/2;  // 1.5x
        credits+=bet+win;   // return bet + winnings
        snprintf(msg,sizeof(msg),"Blackjack! +%d",win); msgCol=C_GOLD; sndWin();
    } else if (dBJ&&!pBJ) {
        snprintf(msg,sizeof(msg),"Dealer BJ. You lose."); msgCol=C_DIM; sndLose();
    } else if (dv>21) {
        credits+=bet*2; snprintf(msg,sizeof(msg),"Dealer busts! +%d",bet*2); msgCol=C_GRN; sndWin();
    } else if (p>dv) {
        credits+=bet*2; snprintf(msg,sizeof(msg),"You win! +%d",bet*2); msgCol=C_GRN; sndWin();
    } else if (p==dv) {
        credits+=bet; snprintf(msg,sizeof(msg),"Push. Bet returned."); msgCol=C_TXT;
    } else {
        snprintf(msg,sizeof(msg),"Dealer wins."); msgCol=C_DIM; sndLose();
    }
    bjState=RESULT;
}

static void dealerPlay() {
    drawHands(false); delay(500);
    while (handTotal(dHand,dCount)<17 && dCount<11) {
        dHand[dCount++]=dealCard(); sndCard();
        drawHands(false); delay(400);
    }
    evalHand();
    drawHUD(); drawMsg(); drawFooter();
    if (credits<=0) {
        snprintf(msg,sizeof(msg),"BROKE! SPC=reset Q=menu"); msgCol=C_RED; drawMsg();
    }
}

static void deal() {
    if (credits<bet) { snprintf(msg,sizeof(msg),"Not enough credits!"); msgCol=C_RED; drawMsg(); return; }
    credits-=bet; origBet=bet;
    pCount=0; dCount=0;
    pHand[pCount++]=dealCard(); sndCard();
    dHand[dCount++]=dealCard(); sndCard();
    pHand[pCount++]=dealCard(); sndCard();
    dHand[dCount++]=dealCard(); sndCard();
    msg[0]='\0'; bjState=PLAYER_TURN;
    drawHUD(); drawHands(true); drawMsg(); drawFooter();
    if (handTotal(pHand,pCount)==21) { dealerPlay(); }  // natural BJ
}

void init() {
    credits=100; bet=5; origBet=5; msg[0]='\0'; msgCol=C_DIM;
    bjState=BETTING;
    shuffleDeck();
    M5Cardputer.Speaker.setVolume(soundVol);
    auto& d=M5Cardputer.Display;
    d.fillScreen(C_BG);
    drawHeader(); drawHUD();
    d.fillRect(0,29,240,66,C_BG);
    snprintf(msg,sizeof(msg),"Place your bet and deal!"); msgCol=C_DIM;
    drawMsg(); drawFooter();
}

bool tick() {
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        auto st=M5Cardputer.Keyboard.keysState();
        bool wantNext=st.enter;
        for (char c : st.word) {
            if (c=='q'||c=='Q') return false;
            if (c=='m'||c=='M') { soundOn=!soundOn; drawFooter(); }
            if (c==']') { soundVol=min(soundVol+50,250); M5Cardputer.Speaker.setVolume(soundVol); drawFooter(); }
            if (c=='[') { soundVol=max(soundVol-50, 50); M5Cardputer.Speaker.setVolume(soundVol); drawFooter(); }

            if (bjState==BETTING) {
                if (c==' ') wantNext=true;
                if (c=='+'||c=='=') { bet=min(bet+1,20); drawHUD(); }
                if (c=='-')        { bet=max(bet-1,1);  drawHUD(); }
            } else if (bjState==PLAYER_TURN) {
                if (c=='h'||c=='H') {
                    if (pCount<11) {
                        pHand[pCount++]=dealCard(); sndCard(); drawHands(true);
                        if (handTotal(pHand,pCount)>21) { dealerPlay(); }
                    }
                }
                if (c=='s'||c=='S') { dealerPlay(); }
                if ((c=='d'||c=='D') && pCount==2 && credits>=bet) {
                    credits-=bet; bet*=2;
                    pHand[pCount++]=dealCard(); sndCard();
                    drawHUD(); drawHands(true);
                    dealerPlay();
                }
            } else if (bjState==RESULT) {
                if (c==' ') wantNext=true;
            }
        }
        if (wantNext) {
            if (bjState==BETTING) { deal(); }
            else if (bjState==RESULT) {
                if (credits<=0) { credits=100; bet=min(origBet,5); }
                bet=origBet;
                bjState=BETTING; msg[0]='\0';
                M5Cardputer.Display.fillRect(0,29,240,66,C_BG);
                drawHUD(); drawMsg(); drawFooter();
            }
        }
    }
    delay(50);
    return true;
}

} // namespace Blackjack
