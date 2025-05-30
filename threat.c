#include "chess.h"


#define canCheckVal 4


 
int SqAttack( int sq, int c );

int Threat(int p, int sq, int c){
#define GET_CAP_PAWN\
        if( g.color[to] == (c^1) ){                 \
          int val = a_value[g.pos[ to ]];             \
          if(promote)                               \
            m = max( m, (VALUE_Q - VALUE_P) + val );\
          else{                                     \
                m = max( m, val );                  \
          }                                         \
        } 
 int maxVal = 0;
 int checkVal = 0;
 int atk_p,to;
 const a_value[] = {0,1,3,3,6,10,20};


 
 if(p==PAWN)
 {
    int x = COLUMN(sq);
    int y = ROW(sq);
    int m = 0;
    if(c == WHITE){
      int promote = (y == 1);
      if(promote && g.color[sq-8]==NEUTRAL)
          m = (VALUE_Q - VALUE_P);
      if( x > 0 )
      {
        to = sq - 8 - 1;  
        GET_CAP_PAWN;
      }
      if( x < 7 )
      {
        to = sq - 8 + 1;  
        GET_CAP_PAWN;
      }      
    }else{ //BLACK
      int promote = (y == 6);
      if(promote && g.color[sq+8]==NEUTRAL)
          m = (VALUE_Q - VALUE_P);
      if( x > 0 )
      {
        to = sq + 8 - 1;  
        GET_CAP_PAWN;
      }
      if( x < 7 )
      {
        to = sq + 8 + 1;  
        GET_CAP_PAWN;
      }          
 
    }
    maxVal = m;
 }else{ // NOT PAWNS
      int n0,d,j,n1,u;
      int c1 = c^1;
       
        static int atk[64];
        static int atkId;
 
        if(++atkId == 0) atkId++;
      
      
        n0 = MAP( sq );
        for ( j = d_start[p]; j <= d_stop[p]; j++ )
        {
          d = dir[j];
          for ( n1 = n0 + d; ( n1 & 0x88 ) == 0; n1 += d )
          {
            u = UNMAP( n1 );
            if ( g.color[u] == NEUTRAL ){
              atk[u] = atkId;
            }else{
              if ( g.color[u] == c1 )
              {
                 int cap_p = g.pos[u];
                 maxVal = max(maxVal,a_value[cap_p]); 
                 atk[u] = atkId;
              }
              break;
            }
            if(!SWEEP(p)) break;
          }
        }
        ////////////
      if(p != KING)  
        for ( j = d_start[p]; j <= d_stop[p]; j++ )
        {
          d = dir[j];
          for ( n1 = n0 + d; ( n1 & 0x88 ) == 0; n1 += d )
          {
            u = UNMAP( n1 );
            if( atk[u] == atkId ){
               checkVal += canCheckVal; 
               goto done;
            }
            if ( g.color[u] != NEUTRAL ) break;
            if(!SWEEP(p)) break;
          }  
        }   
                          
       
 
 }//endif
 
done: 
  return maxVal + checkVal;
}



