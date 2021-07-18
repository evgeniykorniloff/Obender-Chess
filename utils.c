#include "chess.h"
extern int Search_Only_Mtl;

const char *chb[] = {
   "a8","b8","c8","d8","e8","f8","g8","h8",
   "a7","b7","c7","d7","e7","f7","g7","h7",
   "a6","b6","c6","d6","e6","f6","g6","h6",
   "a5","b5","c5","d5","e5","f5","g5","h5",
   "a4","b4","c4","d4","e4","f4","g4","h4",
   "a3","b3","c3","d3","e3","f3","g3","h3",
   "a2","b2","c2","d2","e2","f2","g2","h2",
   "a1","b1","c1","d1","e1","f1","g1","h1"
};


int StrToSq(char *s){
  int j;

  for(j = 0; j < 64; j++)
   if(chb[j][0]==s[0] && chb[j][1]==s[1]) return j;

  return -1;
}

void InitNewGame(void){
   int InitGame(char *);
// InitGame("7r/rp1k1pb1/p1p1p1p1/q1Pp1b1p/P1PP1B2/1Q2P2P/4BPP1/R4RK1 b - a3 0 16 ");
   InitGame("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");

}


int InitGame(char *epd){
  const int hlat[4][21] =
  {
    {'p','n','b','r','q','k',  'P','N','B','R','Q','K',  '1','2','3','4','5','6','7','8',  '/'},
    { BLACK,BLACK,BLACK,BLACK,BLACK,BLACK, WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,  0,  0,  0,  0,  0,  0,  0,  0,   0},
    {PAWN,KNIGHT,BISHOP,ROOK,QUEEN,KING, PAWN,KNIGHT,BISHOP,ROOK,QUEEN,KING, 0,0,0,0,0,0,0,0, 0 },
    { 1,  1,  1,  1,  1,  1,    1,  1,  1,  1,  1,  1,    1,  2,  3,  4,  5,  6,  7,  8,    0}
  };
  char buf[256];
  char *lex;
  int sq,p,c,k,j;
  void InsertPiece(int p, int c, int sq, int i);
  struct Game_Type save;

   if(epd==NULL) return 0;

   save = g;
   //CLEAR GAME
   memset(&g,0,sizeof(struct Game_Type));
   for(j = 0; j < 32; j++) g.list[j] = -1;
   for(j = 0; j < 64; j++){
      g.index[j] = -1;
      g.color[j] = NEUTRAL;
   }
   g.side = WHITE; g.xside = BLACK;
   g.start[BLACK] = g.stop[BLACK] = 16;

   //READ POSITION
   strcpy(buf,epd);
   lex = strtok(buf," \n");
   if(!lex) goto error;

   sq = 0;
   while(*lex){
      for(k = 0; k < 21; k++)
       if(hlat[0][k]==*lex){
         p = hlat[2][k];
         c = hlat[1][k];
         if(p){
           if(sq >= 64  || g.stop[c] >= ((c==WHITE)?16:32) ||
              g.pos[sq]) goto error;
              InsertPiece(p, c, sq, g.stop[c]++);
         }
         sq += hlat[3][k];
         goto nextChar;
       }
      goto error;
 nextChar:
     lex++;
   }
   g.stop[WHITE]--; g.stop[BLACK]--;
   if(g.cnt[WHITE][KING] != 1  ||  g.cnt[BLACK][KING] != 1) goto error;



   //READ SIDE
   lex = strtok(NULL," \n");
   if(!lex) goto success;

   if(lex[1] != '\0') goto error;
   if(*lex=='-');
   else if(*lex=='w'){
      g.side = WHITE; g.xside = BLACK;
   }else if(*lex=='b'){
      g.side = BLACK; g.xside = WHITE;
   }else goto error;



   //READ CASTLING ENABLE
   lex = strtok(NULL," \n");
   if(!lex) goto success;

   if(strcmp(lex,"-")==0);
   else
    while(*lex){
     switch(*lex){
       case 'Q':  g.castl_enable |= WHITE_LEFT_CASTL_ENABLE; break;
       case 'K':  g.castl_enable |= WHITE_RIGHT_CASTL_ENABLE; break;
       case 'q':  g.castl_enable |= BLACK_LEFT_CASTL_ENABLE; break;
       case 'k':  g.castl_enable |= BLACK_RIGHT_CASTL_ENABLE; break;
       default: goto error;
     }
     lex++;
    }

   //READ LAST MOVE SQ (for en passant only)
   lex = strtok(NULL," \n");
   if(!lex) goto success;

   if(strcmp(lex,"-")==0) goto success;
   if((sq=StrToSq(lex)) == -1) goto error;
   if(g.side==WHITE &&  ROW(sq) != 2) goto error;
   if(g.side==BLACK && ROW(sq) != 5) goto error;
   if(g.pos[sq - dP[g.side]] != PAWN  ||  g.color[sq - dP[g.side]] != g.xside ||
      g.pos[sq] != NOPIECE || g.pos[sq+dP[g.side]] != NOPIECE)
     goto error;
   g.__en_pass_sq = sq;


success:
   do{
     extern time_t limit_time;

     limit_time = 7.5*100;
   }while(0);
   return 1;

error:
  g = save;
  return 0;
}


int StrToMove(char *s, Move *ret_mv){
   int len = strlen(s), from, to, find = 0;

   if(len >= 4 && (from = StrToSq(s)) != -1  &&
      (to = StrToSq(s+2)) != -1)   find = 1;
   else if(strcmp(s,"o-o")==0){
       if(g.side==WHITE){
          from = E1; to = G1;
       }else{
          from = E8; to = G8;
       }
       find = 1;
   }else if(strcmp(s,"o-o-o")==0){
       if(g.side==WHITE){
          from = E1; to = C1;
       }else{
          from = E8; to = C8;
       }
       find = 1;
   }


   if(find){
      int LegalMoveList(void);
      int j;
      Move mv;

      LegalMoveList();
      for(j = 0; j < treeCnt[1]; j++){
        Move mv = tree[j];
        if(FROM(mv)==from && TO(mv)==to){
           if(mv & PROMOTE){
              int new_p = QUEEN;
              if(len > 4)
              {
                switch(tolower(s[4])){
                  case 'q':   new_p = QUEEN; break;
                  case 'n':   new_p = KNIGHT; break;
                  case 'b':   new_p = BISHOP; break;
                  case 'r':   new_p = ROOK; break;
                }
              }
              mv = (mv & ~(7<<12)) | (new_p << 12);
            }
           *ret_mv = mv;
           return 1;
        }
      }//
   }
   return 0;
}

int InsertMoveInGame(Move mv){
   unsigned ControlSumm(void *data, unsigned sz);
   if(g.game_cnt < MAX_GAME){
       unsigned temp = ControlSumm(&g,sizeof(struct Game_Type));
       MakeMove(mv);
       UnMakeMove(mv);
       assert(temp==ControlSumm(&g,sizeof(struct Game_Type)));
       MakeMove(mv);
       ply = 0;
       g.game_max = g.game_cnt = g.game_cnt + 1;
       return 1;
   }
   return 0;

}

char *MoveToStr(Move mv, char* str){
   sprintf(str,"%s%s", chb[FROM(mv)], chb[TO(mv)]);
   if(mv & PROMOTE){
     char *chp[] = {"","p","n","b","r","q","k"};
     strcat(str, chp[ PIECE(mv) ]);
   }
   return str;
}





void InitAllVar(void){
 //  void LearnClear(void);
   void HashInit(void);
   void InitEvaluate(void);
 //  LearnClear();
   InitAtk();
   HashInit();
   InitEvaluate();
}


unsigned ControlSumm(void *data, unsigned sz){
   unsigned char *s = data;
   unsigned summ = 0, n = 0;

   while(n < sz){
     summ += *s;
     s++;
     n++;
   }
   return n;
}

char* U64ToStr(char *str, U64 v){
  char *p,*s;
  p = s = str;
  do{
     *p++ = (char)('0' + v%10);

  }while( (v /= 10) > 0);
  *p-- = '\0';
  while(p > s){
    char temp = *p;
    *p = *s;
    *s = temp;
    s++; p--;
  }
  return str;
}

//Example:
//  9 156 1084 48000 Nf3 Nc6 Nc3 Nf6Meaning:
// 9 ply, score=1.56, time = 10.84 seconds, nodes=48000, PV = "Nf3 Nc6 Nc3 Nf6"
void PrintSearchStatus(int ply, int score, int time, U64 nodes, Move *pv){
  char str_nodes[128], str_move[64], output[1024];
 // int kf = VALUE_P / 100,
  int    j;
//  if(kf == 0) kf = 1;

  sprintf(output, "%2d %4d %4d %10s ",ply, score/10 + score%10 /*+ score%VALUE_P*/, time, U64ToStr(str_nodes,nodes));
  for(j = 0; j < 14 && pv[j]; j++) {
    strcat(output, MoveToStr(pv[j],str_move));
    strcat(output," ");
  }

  printf("%s\n", output);
}


/*
  Сортирует массив исходя
*/
