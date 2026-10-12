#include "Chess.h"
#include <ctype.h>
#include <algorithm>
namespace Chess {
Position game;
static uint64_t history[1024];
static int historyCount;
static const int knight[]={33,31,18,14,-14,-18,-31,-33};
static const int bishop[]={15,17,-15,-17};
static const int rook[]={1,-1,16,-16};
static const int king[]={1,-1,16,-16,15,17,-15,-17};
static const int value[]={0,100,320,335,500,900,20000};
static const char *letters=" PNBRQK";
static bool valid(int s) { return s>=0 && s<128 && !(s&0x88); }
void load(Position &p,const char *f) {
  p=Position(); int rank=7,file=0;
  while(*f && *f!=' ') { char c=*f++; if(c=='/') { --rank; file=0; } else if(isdigit(c)) file+=c-'0'; else { const char *v=strchr(letters,toupper(c)); if(v && rank>=0 && file<8) p.board[rank*16+file++]=(v-letters)*(isupper(c)?1:-1); } }
  if(*f) ++f; p.side=*f=='b'?-1:1;
  while(*f && *f!=' ') ++f; if(*f) ++f; p.rights=0;
  while(*f && *f!=' ') { if(*f=='K') p.rights|=1; if(*f=='Q') p.rights|=2; if(*f=='k') p.rights|=4; if(*f=='q') p.rights|=8; ++f; }
  if(*f) ++f;
  if(f[0]>='a' && f[0]<='h' && f[1]>='1' && f[1]<='8') p.ep=(f[1]-'1')*16+f[0]-'a';
  while(*f && *f!=' ') ++f;
  if(*f) { p.halfmove=atoi(++f); while(*f && *f!=' ') ++f; if(*f) p.fullmove=atoi(++f); }
}
static bool attacked(const Position &p,int s,int by) {
  for(int d:{-1,1}) { int t=s-by*16+d; if(valid(t) && p.board[t]==by*PAWN) return true; }
  for(int d:knight) if(valid(s+d) && p.board[s+d]==by*KNIGHT) return true;
  for(int d:king) if(valid(s+d) && p.board[s+d]==by*KING) return true;
  for(int d:king) for(int t=s+d;valid(t);t+=d) { int pc=p.board[t]; if(!pc) continue; if(pc*by>0 && (abs(pc)==QUEEN || abs(pc)==((abs(d)==1 || abs(d)==16)?ROOK:BISHOP))) return true; break; }
  return false;
}
bool check(const Position &p) { for(int s=0;s<128;++s) if(valid(s) && p.board[s]==p.side*KING) return attacked(p,s,-p.side); return true; }
void make(Position &p,Move m) {
  int pc=p.board[m.from],captured=p.board[m.to],side=p.side;
  if(abs(pc)==PAWN && m.to==p.ep && !captured) p.board[m.to-side*16]=0;
  p.board[m.to]=m.promotion?side*m.promotion:pc; p.board[m.from]=0;
  if(abs(pc)==KING) { p.rights&=side==1?12:3; if(m.to==m.from+2) { p.board[m.from+1]=side*ROOK; p.board[m.from+3]=0; } if(m.to==m.from-2) { p.board[m.from-1]=side*ROOK; p.board[m.from-4]=0; } }
  if(m.from==0 || m.to==0) p.rights&=~2; if(m.from==7 || m.to==7) p.rights&=~1;
  if(m.from==112 || m.to==112) p.rights&=~8; if(m.from==119 || m.to==119) p.rights&=~4;
  p.ep=abs(pc)==PAWN && abs(int(m.to)-int(m.from))==32?(m.from+m.to)/2:-1;
  p.halfmove=(abs(pc)==PAWN || captured)?0:p.halfmove+1; if(side==-1) ++p.fullmove; p.side=-side;
}
int legal(const Position &p,Move *out) {
  int n=0;
  auto add=[&](int from,int to,int promo=0) { Move m{uint8_t(from),uint8_t(to),uint8_t(promo)}; Position next=p; make(next,m); next.side=p.side; if(!check(next) && n<256) out[n++]=m; };
  auto pawnMove=[&](int from,int to) { if((to>>4)==0 || (to>>4)==7) { for(int pc:{QUEEN,ROOK,BISHOP,KNIGHT}) add(from,to,pc); } else add(from,to); };
  for(int s=0;s<128;++s) {
    if(!valid(s) || p.board[s]*p.side<=0) continue; int type=abs(p.board[s]);
    if(type==PAWN) {
      int t=s+p.side*16;
      if(valid(t) && !p.board[t]) { pawnMove(s,t); int u=t+p.side*16; if((s>>4)==(p.side==1?1:6) && !p.board[u]) add(s,u); }
      for(int d:{-1,1}) { t=s+p.side*16+d; if(valid(t) && ((p.board[t]*p.side<0 && abs(p.board[t])!=KING) || (t==p.ep && p.board[t-p.side*16]==-p.side*PAWN))) pawnMove(s,t); }
      continue;
    }
    const int *dirs=type==KNIGHT?knight:type==BISHOP?bishop:type==ROOK?rook:king; int count=(type==BISHOP || type==ROOK)?4:8;
    for(int i=0;i<count;++i) for(int t=s+dirs[i];valid(t);t+=dirs[i]) { if(p.board[t]*p.side>0 || abs(p.board[t])==KING) break; add(s,t); if(p.board[t] || type==KNIGHT || type==KING) break; }
    if(type==KING && s==(p.side==1?4:116) && !check(p)) {
      int k=p.side==1?1:4,q=p.side==1?2:8;
      if((p.rights&k) && p.board[s+3]==p.side*ROOK && !p.board[s+1] && !p.board[s+2] && !attacked(p,s+1,-p.side)) add(s,s+2);
      if((p.rights&q) && p.board[s-4]==p.side*ROOK && !p.board[s-1] && !p.board[s-2] && !p.board[s-3] && !attacked(p,s-1,-p.side)) add(s,s-2);
    }
  } return n;
}
String notation(Move m) { String s; s+=char('a'+(m.from&7)); s+=char('1'+(m.from>>4)); s+=char('a'+(m.to&7)); s+=char('1'+(m.to>>4)); if(m.promotion) s+=char(tolower(letters[m.promotion])); return s; }
String fen(const Position &p) {
  String s;
  for(int r=7;r>=0;--r) { int empty=0; for(int f=0;f<8;++f) { int pc=p.board[r*16+f]; if(!pc) ++empty; else { if(empty) s+=String(empty); empty=0; s+=char(pc>0?letters[pc]:tolower(letters[-pc])); } } if(empty) s+=String(empty); if(r) s+='/'; }
  s+=p.side==1?" w ":" b "; if(!p.rights) s+='-'; if(p.rights&1) s+='K'; if(p.rights&2) s+='Q'; if(p.rights&4) s+='k'; if(p.rights&8) s+='q';
  s+=' '; if(p.ep<0) s+='-'; else { s+=char('a'+(p.ep&7)); s+=char('1'+(p.ep>>4)); } s+=' '; s+=String(p.halfmove); s+=' '; s+=String(p.fullmove); return s;
}
bool parse(const String &s,Move &m) { Move moves[256]; int n=legal(game,moves); for(int i=0;i<n;++i) if(notation(moves[i])==s) { m=moves[i]; return true; } return false; }
static uint64_t hash(const Position &p) {
  uint64_t h=1469598103934665603ULL;
  for(int s=0;s<128;++s) if(valid(s)) h=(h^uint8_t(p.board[s]+6))*1099511628211ULL;
  int ep=-1; // Only legal en-passant rights affect repetition identity.
  if(p.ep>=0) { Move moves[256]; int n=legal(p,moves); for(int i=0;i<n;++i) if(moves[i].to==p.ep && abs(p.board[moves[i].from])==PAWN) ep=p.ep; }
  h=(h^uint8_t(p.side+1))*1099511628211ULL; h=(h^p.rights)*1099511628211ULL; return (h^uint8_t(ep+1))*1099511628211ULL;
}
void start() { load(game,"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); historyCount=1; history[0]=hash(game); }
void play(Move m) { make(game,m); if(historyCount==1024) { memmove(history,history+1,1023*sizeof(uint64_t)); --historyCount; } history[historyCount++]=hash(game); }
String result() {
  Move moves[256]; int n=legal(game,moves); if(!n) return check(game)?(game.side==1?"0-1 Schachmatt":"1-0 Schachmatt"):"1/2-1/2 Patt";
  if(game.halfmove>=100) return "1/2-1/2 50-Zuege-Regel";
  int count=0; uint64_t h=hash(game); for(int i=0;i<historyCount;++i) if(history[i]==h) ++count; if(count>=3) return "1/2-1/2 Wiederholung";
  int minors=0,bishops=0,color=-1; bool same=true,other=false;
  for(int s=0;s<128;++s) if(valid(s)) { int pc=abs(game.board[s]); if(pc==KNIGHT || pc==BISHOP) ++minors; if(pc==BISHOP) { ++bishops; int c=((s>>4)+(s&7))&1; if(color>=0 && color!=c) same=false; color=c; } if(pc==PAWN || pc==ROOK || pc==QUEEN) other=true; }
  if(!other && (minors<=1 || (minors==bishops && same))) return "1/2-1/2 Material"; return "*";
}
static uint32_t deadline,nodes;
static bool stopped;
static int evaluate(const Position &p) {
  int score=0;
  for(int s=0;s<128;++s) if(valid(s) && p.board[s]) { int pc=p.board[s],type=abs(pc),rank=pc>0?s>>4:7-(s>>4),file=s&7; int center=7-abs(2*file-7)-abs(2*(s>>4)-7); int bonus=type==PAWN?rank*9+(3-abs(file-3))*3:(type==KNIGHT || type==BISHOP)?center*7:type==KING?-center*3:0; score+=(pc>0?1:-1)*(value[type]+bonus); } return score*p.side;
}
static int ordering(const Position &p,Move m) { return value[abs(p.board[m.to])]*10-value[abs(p.board[m.from])]+m.promotion*200; }
static int search(const Position &p,int depth,int alpha,int beta,int ply,int quiet) {
  ++nodes; if((nodes&255)==0) { delay(1); if(int32_t(millis()-deadline)>=0) stopped=true; } if(stopped) return 0;
  Move moves[256]; int n=legal(p,moves); bool inCheck=check(p); if(!n) return inCheck?-30000+ply:0; if(p.halfmove>=100) return 0;
  bool leaf=depth<=0; if(leaf && quiet<=0) return evaluate(p);
  if(leaf && !inCheck) { int stand=evaluate(p); if(stand>=beta) return beta; alpha=std::max(alpha,stand); }
  std::sort(moves,moves+n,[&](Move a,Move b){return ordering(p,a)>ordering(p,b);});
  for(int i=0;i<n;++i) { Move m=moves[i]; if(leaf && !inCheck && !p.board[m.to] && !m.promotion && !(abs(p.board[m.from])==PAWN && m.to==p.ep)) continue; Position next=p; make(next,m); int score=-search(next,depth-1,-beta,-alpha,ply+1,leaf?quiet-1:quiet); if(stopped) return 0; if(score>=beta) return beta; alpha=std::max(alpha,score); } return alpha;
}
Move best(int depth,uint32_t milliseconds) {
  Move moves[256]; int n=legal(game,moves); if(!n) return {0,0,0}; Move chosen=moves[0]; nodes=0; stopped=false; deadline=millis()+milliseconds;
  for(int d=1;d<=depth;++d) { int alpha=-32000; Move candidate=chosen;
    for(int i=0;i<n;++i) if(notation(moves[i])==notation(chosen)) { std::swap(moves[0],moves[i]); break; }
    for(int i=0;i<n;++i) { Position next=game; make(next,moves[i]); int score=-search(next,d-1,-32000,-alpha,1,3); if(stopped) break; if(score>alpha) { alpha=score; candidate=moves[i]; } }
    if(stopped) break; chosen=candidate; Serial.printf("INFO depth=%d score=%d nodes=%lu pv=%s\n",d,alpha,(unsigned long)nodes,notation(chosen).c_str());
  } return chosen;
}
uint64_t perft(const Position &p,int depth) { if(!depth) return 1; Move moves[256]; int n=legal(p,moves); if(depth==1) return n; uint64_t total=0; for(int i=0;i<n;++i) { Position next=p; make(next,moves[i]); total+=perft(next,depth-1); if(depth>=3) delay(1); } return total; }
bool selftest() {
  struct Test { const char *fen; int depth; uint64_t expected; };
  const Test tests[]={
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",3,8902},
    {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",3,97862},
    {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",3,2812},
    {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",3,9467}};
  bool ok=true; for(const Test &t:tests) { Position p; load(p,t.fen); uint64_t got=perft(p,t.depth); bool pass=got==t.expected; ok&=pass; Serial.printf("TEST perft%d got=%llu expected=%llu %s\n",t.depth,got,t.expected,pass?"PASS":"FAIL"); } return ok;
}
}
