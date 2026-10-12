#include <Arduino.h>
#include <TFT_eSPI.h>
#include "Chess.h"
#include <Preferences.h>
#include <math.h>
SET_LOOP_TASK_STACK_SIZE(32768);
TFT_eSPI tft;
String input,lastMove="--",outcome="*";
bool automatic=true,overflow=false;
int engineSide=-1;
Chess::Move highlight{255,255,0};
constexpr int BX=14, BY=16, CELL=26;
int selected=-1, promotionTo=-1, confirmNew=0;
bool calibrated=false, calibrating=false, touchLatched=true;
int calibrationStep=0;
float mapping[6]={}, candidate[6]={};
uint16_t rawX[3],rawY[3];
const int targetX[4]={20,300,20,300}, targetY[4]={20,20,220,220};
uint32_t lastPoll=0, releaseSince=0;
int samples=0;
uint32_t sampleX=0,sampleY=0;
uint16_t previousX=0,previousY=0;
String touchMessage="Figur antippen";
// Hand-drawn silhouettes, doubled to fit 26 pixel squares.
const uint16_t pieces[6][12]={
 {0,0x060,0x0f0,0x0f0,0x060,0x060,0x0f0,0x0f0,0x1f8,0x3fc,0,0},
 {0x030,0x078,0x1f8,0x3e8,0x3f8,0x030,0x070,0x0e0,0x1f8,0x3fc,0,0},
 {0x060,0x0b0,0x1d8,0x1b8,0x0f0,0x060,0x060,0x0f0,0x1f8,0x3fc,0,0},
 {0x198,0x1f8,0x1f8,0x0f0,0x0f0,0x0f0,0x0f0,0x0f0,0x1f8,0x3fc,0,0},
 {0x264,0x264,0x3fc,0x1f8,0x1f8,0x0f0,0x0f0,0x0f0,0x1f8,0x3fc,0,0},
 {0x060,0x0f0,0x060,0x1f8,0x1f8,0x0f0,0x0f0,0x0f0,0x1f8,0x3fc,0,0}
};
void button(int y,const String &label) {
 tft.fillRoundRect(230,y,88,28,4,0x39E7);
 tft.setTextColor(TFT_WHITE,0x39E7);
 tft.drawCentreString(label,274,y+10,1);
}
void drawBoard(const String &status="") {
 if(calibrating) return;
 tft.fillScreen(0x1082); tft.setTextSize(1);
 Chess::Move moves[256]; int count=selected>=0?Chess::legal(Chess::game,moves):0;
 for(int rank=0;rank<8;++rank) for(int file=0;file<8;++file) {
  int s=rank*16+file,x=BX+file*CELL,y=BY+(7-rank)*CELL;
  uint16_t bg=((rank+file)&1)?0xCE59:0x540B; tft.fillRect(x,y,CELL,CELL,bg);
  if(s==highlight.from || s==highlight.to) tft.drawRect(x,y,CELL,CELL,TFT_YELLOW);
  if(s==selected) { tft.drawRect(x,y,CELL,CELL,TFT_CYAN); tft.drawRect(x+1,y+1,24,24,TFT_CYAN); }
  int pc=Chess::game.board[s];
  if(pc) {
   uint16_t fg=pc>0?TFT_WHITE:TFT_BLACK,edge=pc>0?TFT_BLACK:0xBDF7;
   const uint16_t *bitmap=pieces[abs(pc)-1];
   for(int r=0;r<12;++r) for(int c=0;c<12;++c) if(bitmap[r]&(1<<(11-c))) tft.fillRect(x+c*2,y+1+r*2,4,4,edge);
   for(int r=0;r<12;++r) for(int c=0;c<12;++c) if(bitmap[r]&(1<<(11-c))) tft.fillRect(x+1+c*2,y+2+r*2,2,2,fg);
  }
  for(int i=0;i<count;++i) if(moves[i].from==selected && moves[i].to==s) { tft.fillCircle(x+21,y+21,3,TFT_CYAN); break; }
 }
 tft.setTextColor(TFT_WHITE,0x1082);
 for(int i=0;i<8;++i) { tft.drawString(String(char('a'+i)),BX+10+i*CELL,229,1); tft.drawString(String(8-i),3,BY+9+i*CELL,1); }
 tft.drawString("ESP32 SCHACH",231,5,1);
 tft.drawString("Zug "+String(Chess::game.fullmove),231,23,2);
 tft.drawString(Chess::game.side==1?"Weiss":"Schwarz",231,42,2);
 tft.drawString("Letzter: "+lastMove,231,64,1);
 String message=status.length()?status:touchMessage;
 if(outcome!="*") { tft.drawString(outcome.substring(0,outcome.indexOf(' ')),231,77,1); message=outcome.substring(outcome.indexOf(' ')+1); }
 else if(Chess::check(Chess::game)) tft.drawString("SCHACH!",231,77,1);
 if(promotionTo>=0) {
  button(98,"Dame"); button(132,"Turm"); button(166,"Laeufer"); button(200,"Springer");
 } else {
  button(98,confirmNew==1?"Bestaetigen?":"Neu Weiss");
  button(132,confirmNew==2?"Bestaetigen?":"Neu Schwarz");
  button(166,"Kalibrieren");
  tft.setTextColor(TFT_WHITE,0x1082);
  // Wrap sidebar text to 14 characters per line.
  for(int i=0;i<3 && i*14<int(message.length());++i) tft.drawString(message.substring(i*14,i*14+14),231,201+i*12,1);
 }
}
void calibrationScreen(const String &message="") {
 tft.fillScreen(0x1082); tft.setTextColor(TFT_WHITE,0x1082);
 tft.drawCentreString("Touch kalibrieren",160,78,2);
 tft.drawCentreString("Kreuz "+String(calibrationStep+1)+" von 4 antippen",160,104,2);
 tft.drawCentreString("Danach Finger abheben",160,130,2);
 tft.drawCentreString(message,160,156,1);
 int x=targetX[calibrationStep],y=targetY[calibrationStep];
 tft.drawCircle(x,y,9,TFT_YELLOW); tft.drawFastHLine(x-14,y,29,TFT_YELLOW); tft.drawFastVLine(x,y-14,29,TFT_YELLOW);
}
void beginCalibration() {
 selected=promotionTo=-1; confirmNew=0; calibrating=true; calibrationStep=0;
 touchLatched=true; samples=0; releaseSince=0;
 calibrationScreen(); Serial.println("TOUCH CALIBRATION: tap four crosses; calibrate cancel aborts");
}
void calibrationPoint(uint16_t x,uint16_t y) {
 if(calibrationStep<3) { rawX[calibrationStep]=x; rawY[calibrationStep]=y; }
 if(calibrationStep==2) {
  float ux=int(rawX[1])-int(rawX[0]),uy=int(rawY[1])-int(rawY[0]);
  float vx=int(rawX[2])-int(rawX[0]),vy=int(rawY[2])-int(rawY[0]);
  float det=ux*vy-uy*vx;
  if(fabsf(det)<100000) { calibrationStep=0; calibrationScreen("Punkte zu nah - erneut versuchen"); return; }
  candidate[0]=280*vy/det; candidate[1]=-280*vx/det;
  candidate[2]=20-candidate[0]*rawX[0]-candidate[1]*rawY[0];
  candidate[3]=-200*uy/det; candidate[4]=200*ux/det;
  candidate[5]=20-candidate[3]*rawX[0]-candidate[4]*rawY[0];
 }
 if(calibrationStep==3) {
  float px=candidate[0]*x+candidate[1]*y+candidate[2],py=candidate[3]*x+candidate[4]*y+candidate[5];
  if(fabsf(px-300)>14 || fabsf(py-220)>14) { calibrationStep=0; calibrationScreen("Kontrolle ungenau - erneut versuchen"); return; }
  memcpy(mapping,candidate,sizeof(mapping)); calibrated=true; calibrating=false;
  Preferences prefs; bool saved=false;
  if(prefs.begin("chess-touch",false)) { saved=prefs.putBytes("rot3-v1",mapping,sizeof(mapping))==sizeof(mapping); prefs.end(); }
  Serial.println(saved?"TOUCH CALIBRATED SAVED":"TOUCH CALIBRATED (storage failed)");
  touchMessage=saved?"Figur antippen":"Speichern fehlgeschlagen"; drawBoard(); return;
 }
 ++calibrationStep; calibrationScreen();
}
void report() {
 Serial.println("FEN "+Chess::fen(Chess::game)); Serial.println("RESULT "+outcome);
 Serial.printf("STATE side=%s check=%d heap=%u\n",Chess::game.side==1?"white":"black",Chess::check(Chess::game),ESP.getFreeHeap());
}
void movePlayed(Chess::Move m,const char *who) {
 selected=promotionTo=-1; confirmNew=0; touchMessage="Figur antippen"; lastMove=Chess::notation(m); highlight=m; Chess::play(m); outcome=Chess::result();
 Serial.printf("%s %s\n",who,lastMove.c_str()); drawBoard(); report();
}
void engineMove() { if(outcome!="*") return; drawBoard("ESP32 denkt ..."); Chess::Move m=Chess::best(4,1800); movePlayed(m,"ENGINE"); }
void command(String s,bool emitReady=true) {
 s.trim(); s.toLowerCase();
 if(s=="help") Serial.println("COMMANDS: new [white|black], e2e4, e7e8q, board, legal, go, auto on|off, resign, selftest, perft 1..4, calibrate, calibrate cancel, touchinfo, tap X Y, help");
 else if(s=="new" || s=="new white" || s=="new black") { selected=promotionTo=-1; confirmNew=0; touchMessage="Figur antippen"; Chess::start(); outcome="*"; lastMove="--"; highlight={255,255,0}; automatic=true; engineSide=s=="new black"?1:-1; drawBoard(); report(); if(engineSide==1) engineMove(); }
 else if(s=="calibrate") beginCalibration();
 else if(s=="calibrate cancel") { calibrating=false; touchMessage=calibrated?"Figur antippen":"Touch unkalibriert"; drawBoard(); }
 else if(s=="touchinfo") Serial.printf("TOUCH calibrated=%d calibrating=%d step=%d pressure=%u rotation=3 selected=%d promotion=%d\n",calibrated,calibrating,calibrationStep,tft.getTouchRawZ(),selected,promotionTo);
 else if(s.startsWith("tap ")) { int x,y; char extra; if(sscanf(s.c_str(),"tap %d %d %c",&x,&y,&extra)==2 && x>=0 && x<320 && y>=0 && y<240) handleTap(x,y); else Serial.println("ERROR tap X Y (0..319, 0..239)"); }
 else if(s=="board") report();
 else if(s=="legal") { Chess::Move moves[256]; int n=Chess::legal(Chess::game,moves); Serial.print("LEGAL"); for(int i=0;i<n;++i) { Serial.print(' '); Serial.print(Chess::notation(moves[i])); } Serial.println(); }
 else if(s=="auto on") { automatic=true; Serial.println("OK auto on"); if(Chess::game.side==engineSide) engineMove(); }
 else if(s=="auto off") { automatic=false; Serial.println("OK auto off"); }
 else if(s=="go") engineMove();
 else if(s=="resign" && outcome=="*") { outcome=Chess::game.side==1?"0-1 Aufgabe":"1-0 Aufgabe"; drawBoard(); report(); }
 else if(s=="selftest") { drawBoard("Regeltest ..."); Serial.println(Chess::selftest()?"SELFTEST PASS":"SELFTEST FAIL"); drawBoard(); }
 else if(s.startsWith("perft ") && s.length()==7 && s[6]>='1' && s[6]<='4') Serial.printf("PERFT %llu\n",Chess::perft(Chess::game,s[6]-'0'));
 else { Chess::Move m; if(outcome!="*") Serial.println("ERROR Spiel beendet; new startet neu"); else if(automatic && Chess::game.side==engineSide) Serial.println("ERROR ESP32 ist am Zug; go"); else if(!Chess::parse(s,m)) Serial.println("ERROR Ungueltiger Zug/Befehl; help oder legal"); else { movePlayed(m,"MOVE"); if(automatic && outcome=="*" && Chess::game.side==engineSide) engineMove(); } }
 if(emitReady) Serial.println("READY");
}
void handleTap(int x,int y) {
 if(calibrating) { Serial.println("ERROR Kalibrierung aktiv; calibrate cancel"); return; }
 if(promotionTo>=0) {
  if(x>=230) {
   const int types[]={Chess::QUEEN,Chess::ROOK,Chess::BISHOP,Chess::KNIGHT};
   for(int i=0;i<4;++i) if(y>=98+34*i && y<126+34*i) { Chess::Move m{uint8_t(selected),uint8_t(promotionTo),uint8_t(types[i])}; command(Chess::notation(m),false); return; }
  }
  return;
 }
 if(x>=230) {
  int choice=(y>=98 && y<126)?1:(y>=132 && y<160)?2:0;
  if(choice) { if(confirmNew==choice) command(choice==1?"new white":"new black",false); else { confirmNew=choice; drawBoard(); } return; }
  if(y>=166 && y<194) { beginCalibration(); return; }
  return;
 }
 confirmNew=0;
 if(outcome!="*" || (automatic && Chess::game.side==engineSide)) return;
 if(x<BX || x>=BX+8*CELL || y<BY || y>=BY+8*CELL) return;
 int square=(7-(y-BY)/CELL)*16+(x-BX)/CELL;
 if(square==selected) { selected=-1; touchMessage="Figur antippen"; drawBoard(); return; }
 if(Chess::game.board[square]*Chess::game.side>0) { selected=square; touchMessage="Zielfeld antippen"; drawBoard(); return; }
 if(selected<0) return;
 Chess::Move moves[256]; int n=Chess::legal(Chess::game,moves);
 for(int i=0;i<n;++i) if(moves[i].from==selected && moves[i].to==square) {
  if(moves[i].promotion) { promotionTo=square; drawBoard(); return; }
  command(Chess::notation(moves[i]),false); return;
 }
 touchMessage="Zug ungueltig"; drawBoard();
}
void pollTouch() {
 uint32_t now=millis(); if(now-lastPoll<15) return; lastPoll=now;
 uint16_t z=tft.getTouchRawZ();
 if(z<200) {
  samples=0;
  if(!releaseSince) releaseSince=now;
  if(now-releaseSince>=100) touchLatched=false;
  return;
 }
 releaseSince=0;
 if(z<350 || touchLatched) return;
 uint16_t x,y; tft.getTouchRaw(&x,&y);
 if(!samples || abs(int(x)-int(previousX))>30 || abs(int(y)-int(previousY))>30) { samples=0; sampleX=sampleY=0; }
 previousX=x; previousY=y; sampleX+=x; sampleY+=y;
 if(++samples<8) return;
 x=sampleX/samples; y=sampleY/samples; samples=0; touchLatched=true;
 if(calibrating) { calibrationPoint(x,y); return; }
 if(!calibrated) { beginCalibration(); return; }
 int px=lroundf(mapping[0]*x+mapping[1]*y+mapping[2]),py=lroundf(mapping[3]*x+mapping[4]*y+mapping[5]);
 if(px>=0 && px<320 && py>=0 && py<240) { Serial.printf("TOUCH TAP %d %d\n",px,py); handleTap(px,py); }
}
void setup() {
 Serial.begin(115200); input.reserve(80); pinMode(32,OUTPUT); digitalWrite(32,HIGH);
 tft.init(); tft.setRotation(3); Chess::start();
 Preferences prefs;
 if(prefs.begin("chess-touch",true)) {
  if(prefs.getBytesLength("rot3-v1")==sizeof(mapping)) {
   prefs.getBytes("rot3-v1",mapping,sizeof(mapping)); calibrated=true;
   for(float v:mapping) if(!isfinite(v)) calibrated=false;
  }
  prefs.end();
 }
 drawBoard(); if(!calibrated) beginCalibration();
 Serial.println("ESP32 CHESS 2.0 / TOUCH / 115200 / human=white engine=black");
 Serial.printf("DISPLAY %dx%d rotation=3 CS=%d DC=%d RST=%d TOUCH_CS=%d\n",tft.width(),tft.height(),TFT_CS,TFT_DC,TFT_RST,TOUCH_CS); report(); Serial.println("READY");
}
void loop() {
 while(Serial.available()) { char c=Serial.read(); if(c=='\r') continue; if(c=='\n') { if(overflow) Serial.println("ERROR Zeile zu lang\nREADY"); else if(input.length()) command(input); input=""; overflow=false; } else if(input.length()<79) input+=c; else overflow=true; }
 pollTouch(); delay(2);
}
