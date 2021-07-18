#include "chess.h"

HashKey hashRnd[2][8][64];

typedef struct{
  HashKey key,key1;
  int score,flag,depth;
  Move mv;
}HItem;

const int Hash_Size = (1<<20);


HItem *hash[2];


U64 Rand64(void){

 return      ((U64)rand() << 60) ^
             ((U64)rand() << 45) ^
             ((U64)rand() << 30) ^
             ((U64)rand() << 15) ^
             ((U64)rand());
}

void HashInit(void){
    int c, p, sq;



    for(c = WHITE; c <= BLACK; c++){
       hash[c] = malloc(sizeof(HItem)*Hash_Size);
       assert(hash[c] != NULL);
       for(p = 0; p <8 ; p++)
        for(sq = 0; sq < 64; sq++)
          hashRnd[c][p][sq] = Rand64();

    }
}

void HashClear(void){
    memset(hash[WHITE],0,sizeof(HItem) * Hash_Size);
    memset(hash[BLACK],0,sizeof(HItem) * Hash_Size);
}


int HashLook(int alpha,int beta, int depth, int *ret_score, Move *ret_mv){
   HItem *p = (HItem*)&hash[g.side][(int)g.key & (Hash_Size-1)];

   if(p->key==g.key && p->key1==g.key1){
      *ret_mv = p->mv;
      if(p->depth >= depth && p->score>-INF+100  && p->score<INF-100)
        switch(p->flag){
          case ALPHA:
             if(p->score <= alpha){
               *ret_score = p->score;
               return 1;
             }
          break;
          case EXACT:

               *ret_score = p->score;
               return 1;


          case BETA:
             if(p->score >= beta){
               *ret_score = p->score;
               return 1;
             }
          break;
        }

   }

   return 0;
}

int HashEmptyNode(int c, HashKey key, int depth){
 HItem *p = (HItem*)&hash[c][(int)key & (Hash_Size-1)];
 return p->key != key && p->depth <= depth;

}

void HashInsert(int alpha, int beta, int depth, Move mv){
 HItem *p = (HItem*)&hash[g.side][(int)g.key & (Hash_Size-1)];
 int flag;

 if(mv){
  flag = alpha < beta ? EXACT : BETA;
 }else flag  = ALPHA;

 if(
      p->flag == EMPTY  ||
      p->flag*2 + p->depth <= flag*2 + depth
   //  p->depth<=depth
   ){
    p->score = alpha;
    p->depth = depth;
    p->key   = g.key;
    p->key1   = g.key1;
    p->flag  = flag;
    p->mv = mv;
 }
}


int HashDamp(void){
  int c,j,full=0;

  for(c = WHITE; c <= BLACK; c++){
   for(j = 0; j < Hash_Size; j++)
     if(hash[c][j].depth)
       full++;
  }
  return  full * 100 / (Hash_Size*2);
}
