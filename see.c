#include "chess.h"

/*
  попытка получить результат размена по полю
*/

    int GetMinPieceAtk(int c, int sq);
    int See_Helper( int a, int b, int sq, int c, 
                    int p_sq, int score[2] );

/************ результат размена по полю *************/
int See( Move mv, int c){
 int from = FROM(mv);
 int to = TO(mv);
 int p = PIECE(mv);
 int result;
 int score[2] = {0,0}, 
     cap_val,
     a,b;
 if(mv&EN_PASSANT) cap_val = VALUE_P;
 else cap_val = value[g.pos[to]]; 
 b = cap_val;       // max result
 a = -value[p] + cap_val; // min result

 score[c] += cap_val;
 g.color[from] = NEUTRAL;
 
 result = -See_Helper( -b, -a, to, c^1, p, score ); 
 
 g.color[from] = c;
 return result;
}


/* ищет наименьшуб фигуру, бьющую поле, 0..63 or -1 */
int GetMinPieceAtk(int c, int sq){
  int Attack(int p, int c, int from, int to);
  int min_piece = KING + 1;
  int min_from = -1;
  int j;
  
  for( j = g.start[c]; j <= g.stop[c]; j++)
  {
    int from = g.list[j];
    if( j != -1 
        && g.color[from]==c 
        && g.pos[from] < min_piece
        && Attack( g.pos[from], c, from, sq) )
    {
      min_piece = g.pos[from];
      min_from = from;
      if( min_piece == PAWN ) break;    
    }    
  }
  return min_from;
}

/* бьет наименьшей фигурой */
int See_Helper( int a, int b, int sq, int c, 
                int p_sq, int score[2] )
{
  int from;
  a = max(a, score[c] - score[c^1]);
  if(a >= b) return min(a,b);
  from = GetMinPieceAtk(c,sq);
  if(from==-1) return a;
  g.color[from] = NEUTRAL;
  score[c] += value[p_sq];
  a = max(a, -See_Helper(-b,-a,sq,c^1, g.pos[from], score));
  g.color[from] = c;
  return min(a,b);
}



