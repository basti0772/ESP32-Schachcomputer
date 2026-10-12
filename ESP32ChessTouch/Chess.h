#pragma once
#include <Arduino.h>
namespace Chess {
enum { PAWN=1, KNIGHT, BISHOP, ROOK, QUEEN, KING };
struct Move { uint8_t from, to, promotion; };
struct Position { int8_t board[128]={}; int side=1,rights=15,ep=-1,halfmove=0,fullmove=1; };
extern Position game;
void load(Position &p,const char *fen);
void start();
bool check(const Position &p);
void make(Position &p,Move m);
int legal(const Position &p,Move *moves);
String notation(Move m);
String fen(const Position &p);
bool parse(const String &s,Move &m);
Move best(int depth,uint32_t milliseconds);
void play(Move m);
String result();
uint64_t perft(const Position &p,int depth);
bool selftest();
}
