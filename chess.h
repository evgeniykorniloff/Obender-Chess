#ifndef CHESS_H_INCLUDED
#define CHESS_H_INCLUDED
//////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>

///// system def.//////////
#ifdef __BORLANDC__
  typedef unsigned __int64 U64; // CBuilder unsigned 64-bit
  typedef __int64   I64;
#else
  typedef unsigned long long U64; // CNU C unsigned 64-bit
  typedef long long   I64;
#endif
///////////////////////////


#define NOPIECE 0
#define PAWN    1
#define KNIGHT  2
#define BISHOP  3
#define ROOK    4
#define QUEEN   5
#define KING    6

#define VALUE_P  1000
#define VALUE_N  (3*VALUE_P)
#define VALUE_B  (3*VALUE_P)
#define VALUE_R  (5*VALUE_P)
#define VALUE_Q  (10*VALUE_P)
#define VALUE_K  (1000*VALUE_P)
#define INF      VALUE_K

#define NEUTRAL 2
#define WHITE   0
#define BLACK   1

#define ROW(sq)     (int)((sq)>>3)
#define COLUMN(sq)  (int)((sq)&7)
#define MAP(sq)     (int)(((sq&~7)<<1) | ((sq)&7))
#define UNMAP(sq)   (int)(((sq&~7)>>1) | ((sq)&7))

typedef unsigned int Move;
typedef U64 HashKey;

#define TO(mv)     (int)((mv)&63)
#define FROM(mv)   (int)(((mv)>>6)&63)
#define PIECE(mv)  (int)(((mv)>>12)&7)
//#define CAP_PIECE(mv)  (((mv)>>15)&7)

#define PROMOTE       ((Move)1 << 31)
#define CAPTURE       ((Move)1 << 30)
#define EN_PASSANT   ((Move)1 << 29)
#define LEFT_CASTL    ((Move)1 << 28)
#define RIGHT_CASTL   ((Move)1 << 27)

#define MAX_GAME 400
#define MAX_PLY  100

#define WHITE_LEFT_CASTL_ENABLE  (1<<0)
#define WHITE_RIGHT_CASTL_ENABLE (1<<1)
#define BLACK_LEFT_CASTL_ENABLE  (1<<2)
#define BLACK_RIGHT_CASTL_ENABLE (1<<3)

#define LEFT_CASTL_ENABLE(c) (g.castl_enable & (WHITE_LEFT_CASTL_ENABLE<<(c<<1)))
#define RIGHT_CASTL_ENABLE(c) (g.castl_enable & (WHITE_RIGHT_CASTL_ENABLE<<(c<<1)))

#define SWEEP(p)   ((1<<(p)) &  ((1<<ROOK)|(1<<QUEEN)|(1<<BISHOP)))

//#define SWAP(a,b) (((a) == (b)) || (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b))))
#define SWAP(a,b) {int tmp = a; a = b; b = tmp;}
typedef struct {
  int cap_sq,cap_p, cap_p_i, castl_enable;
} Tag_Move;



struct Game_Type{
  int pos[64],color[64],index[64],list[32],start[2],stop[2],
      cnt[2][8], mtl[2], st_score[2],  pawn_column_cnt[2][8],
      pawn_row_cnt[2][8],castl_enable,side,xside,
      casling[2],
      kingSq[2],__en_pass_sq;
  HashKey key,key1;

  int game_cnt,game_max;
  Move game_list[MAX_GAME + MAX_PLY];
  HashKey key_list[MAX_GAME + MAX_PLY];
  Tag_Move tag_list[MAX_GAME + MAX_PLY];
  int white_mtl_list[MAX_GAME + MAX_PLY];
  int white_st_score_list[MAX_GAME + MAX_PLY];
  int isLearn;
};

extern  int treeCnt[MAX_PLY];
extern  Move tree[MAX_PLY * 100];
extern  int  sort_val[MAX_PLY * 100];

typedef Move Line[MAX_PLY];
enum{ EMPTY,ALPHA,BETA,EXACT};


typedef struct{
  int score,flag,depth;
  Line line;
  Move mv;
}Root_Score;


enum squares  {
 A8,B8,C8,D8,E8,F8,G8,H8,
 A7,B7,C7,D7,E7,F7,G7,H7,
 A6,B6,C6,D6,E6,F6,G6,H6,
 A5,B5,C5,D5,E5,F5,G5,H5,
 A4,B4,C4,D4,E4,F4,G4,H4,
 A3,B3,C3,D3,E3,F3,G3,H3,
 A2,B2,C2,D2,E2,F2,G2,H2,
 A1,B1,C1,D1,E1,F1,G1,H1
};


extern int value[7];
extern int score_table[2][8][64];
extern HashKey hashRnd[2][8][64];
extern int ply;
extern struct Game_Type g;
extern const int dP[2], prom_row[2], first_pawn_row[2],d_start[7],d_stop[7],
       dir[16];


void assert(int expr);
int Generate(void);
void InsertPiece(int p, int c, int sq, int i);
void RemovePiece(int p, int c, int sq, int i);
void MakeMove(Move mv);
void UnMakeMove(Move mv);
void InitAtk(void);
int Attack(int p, int c, int from, int to);
int SqAttack(int sq, int c);
int LegalMoveList(void);
int InsertMoveInGame(Move mv);
void InitAllVar(void);
Move LastMove(void);
int Quies( int alpha, int beta );
int Evaluate(int alpha, int beta);
int StopSearch( void );

#undef max
#undef abs
#undef min
int max(int v1, int v2);
int min(int v1, int v2);

extern int is_stop_search;
extern time_t start_time, curr_time, limit_time;
extern int is_stop_search;
extern U64 cnt_nodes;

void PrintSearchStatus(int ply, int score, int time, U64 nodes, Move *pv);
void Pick( int low, int high );
void SavePvLine( Line dest, Line source, Move mv );
void TimeReset( void );
void HistoryInit( void );
void InitEvaluate( void );
///////////////////////////////////
#endif




