#include "chess.h"

int value[7] = {0,VALUE_P,VALUE_N,VALUE_B,VALUE_R,VALUE_Q,VALUE_K};
int score_table[2][8][64];
extern int history[2][8][64];


#define ROOK_BONUS   40
#define KNIGHT_BONUS 50
#define BISHOP_BONUS 60
#define QUEEN_BONUS  110

const int add_st_value[7] = {0,0,KNIGHT_BONUS, BISHOP_BONUS,ROOK_BONUS, QUEEN_BONUS};

const int DOUBLE_PAWN = 6;
const int ISOL_PAWN   = 12;
//const int ISOL_PAWN_COLUMN[8] = {0,1,2,3,3,2,1,0};
//const ISOL_PAWN[] = { 0,8,16,24,26,26,26,26,26,26,26,26,26,26,26};
const int HALF_PASSED_PAWN = 1;
const int PASSED_PAWN = 26; //6 + 12 + 4;
const int PASSED_PAWN_ROW[2][8] =
{
//  {0,32,16,8,4,2,1,0},
//  {0,1,2,4,8,16,32,0}
  {0,64,32,16,8,4,2,0},
  {0,2,4,8,16,32,64,0}

};
const int BACK_PAWN = 2;
const int BAD_PAWN_ATK = 3;
const int PIN = 8;
const int XRAY = 6;
const int HUNG[] = {0,10,3,3,5,10,20};
const  int comp_value[] = {0,1,3,3,5,10,20};
#define CNTRL_PAWN   (1<<15)
#define CNTRL_KNIGHT (1<<14)
#define CNTRL_BISHOP (1<<13)
#define CNTRL_ROOK   (1<<12)
#define CNTRL_QUEEN  (1<<11)
#define CNTRL_KING   (1<<10)
const int piece_cntrl[] = {0,CNTRL_PAWN,CNTRL_KNIGHT,CNTRL_BISHOP,
                           CNTRL_ROOK,CNTRL_QUEEN,CNTRL_KING};
const int comp_piece_cntrl[] = {0,CNTRL_PAWN,CNTRL_KNIGHT,CNTRL_KNIGHT,
                           CNTRL_ROOK,CNTRL_QUEEN,CNTRL_KING};
const int pawn_start_row[8] = {NEUTRAL, BLACK, NEUTRAL, NEUTRAL,
                               NEUTRAL, NEUTRAL,WHITE,NEUTRAL};
#define PAWN_START_POS(sq,c)  ( pawn_start_row[ROW(sq)]==c )
const int opponent_board[] = {WHITE,WHITE,WHITE,WHITE,
                              BLACK,BLACK,BLACK,BLACK};
const int main_diag[64] = {
  2,1,0,0,0,0,1,2,
  1,2,1,0,0,1,2,1,
  0,1,2,1,1,2,1,0,
  0,0,1,2,2,1,0,0,
  0,0,1,2,2,1,0,0,
  0,1,2,1,1,2,1,0,
  1,2,1,0,0,1,2,1,
  2,1,0,0,0,0,1,2

};

struct Atk_Type{
  int bit_atk[64];
  //double math_atk[64];

} p_attack[2];



int max(int v1, int v2) {  return v1 > v2 ? v1 : v2; }
int min(int v1, int v2) {  return v1 < v2 ? v1 : v2; }



//int abs(int v){  return v >= 0 ? v : (-v);}
int Distance(int v1, int v2){

  return max(abs(ROW(v1)-ROW(v2)),
             abs(COLUMN(v1)-COLUMN(v2)));
}

int Taxi(int sq0, int sq1)
{
   return abs(COLUMN(sq0)-COLUMN(sq1)) + abs(ROW(sq0)-ROW(sq1));
}




/*
  Степень фигурной угрозы противника (0..1)
*/
double Stage(int c){
   const int EMTL = 14 + 2*6 + 2*4 + 2*3;
    int c1 = c^1;
    int emtl = g.cnt[c1][QUEEN]*14 +
               g.cnt[c1][ROOK]*6 +
               g.cnt[c1][BISHOP]*4 +
               g.cnt[c1][KNIGHT]*3;

    if(emtl <= 4) emtl = 0;
    if(emtl > EMTL) emtl = EMTL;
    return (double)emtl / EMTL;
}

double StageKnightBishop(int c){
   const int EMTL =  2*4 + 2*3;   // 14
   int *v = g.cnt[c ^ 1],
        cnt  =  v[BISHOP] + v[KNIGHT],
        emtl = v[BISHOP]*4 +
               v[KNIGHT]*3;
    if( cnt <= 1 ) return 0;
    if( cnt == 2 ) emtl -= 2;

    if(emtl > EMTL) emtl = EMTL;
    return (double)emtl / EMTL;

}



void InitEvaluate(void){
 void  GenScoreTable(int p, int tbl[64], int c, int max, int op_king_sq);
 int p,c,j;
  const int blackPawnScore[64] =  // Jon Stanback pawn score
   { 0, 0, 0, 0, 0, 0, 0, 0,
     4, 4, 4, 0, 0, 4, 4, 4,
     6, 8, 2,10,10, 2, 8, 6,
     6, 8,12,16,16,12, 8, 6,
     8,12,16,24,24,16,12, 8,
    12,16,24,32,32,24,16,12,
    12,16,24,32,32,24,16,12,
     0, 0, 0, 0, 0, 0, 0, 0};


 const int MaxTableValue[] = {0,32,34,32,32,32,36};

 for(c = WHITE; c <= BLACK; c++){
   for(p = PAWN; p <= KING; p++)
      GenScoreTable(p,score_table[c][p],c,MaxTableValue[p],
      g.kingSq[c^1]);

 }

 for(j = 0; j < 64; j++){
    score_table[BLACK][PAWN][j] =  blackPawnScore[j];
    score_table[WHITE][PAWN][j] =  blackPawnScore[63-j];
 }


 for(c = WHITE; c <= BLACK; c++){
    const int king_file[] = {0,0,3,4,4,3,0,0};
    double stage = Stage(c);
    int j;


    if(g.cnt[c][PAWN]==0) stage = 0; //король стремится только в центр

    for(j = 0; j < 64; j++){
      double tmp = score_table[c][KING][j];
      if(stage > 0.5) tmp += king_file[COLUMN(j)];

      score_table[c][KING][j] =  -tmp * stage  +  tmp * (1.0 - stage);
    }
 }

 /*
 do{
    void PrintArray(FILE*f,int tbl[64], int p, int c);
    FILE *f = fopen("sctable.txt","w");
    int p,c;

    for(c = WHITE; c <= BLACK; c++)
     for(p = PAWN; p <= KING; p++)
       PrintArray(f, score_table[c][p], p, c);

    fclose(f);

 }while(0);
 */
}

int maxHungValue[2];
int Evaluate(int alpha, int beta){
  const int MARGIN = 500;

  int s[2];

  maxHungValue[WHITE] = maxHungValue[BLACK] = 0;

  s[WHITE] = g.mtl[WHITE];
  s[BLACK] = g.mtl[BLACK];
/***
  if( (g.cnt[WHITE][PAWN]  | g.cnt[BLACK][PAWN])==0  &&
      (g.cnt[WHITE][QUEEN] | g.cnt[BLACK][QUEEN])==0 &&
      (g.cnt[WHITE][ROOK ] | g.cnt[BLACK][ROOK])==0  &&
       g.cnt[WHITE][BISHOP] * 4   +   g.cnt[WHITE][KNIGHT] * 3   <  4+3 &&
       g.cnt[BLACK][BISHOP] * 4   +   g.cnt[BLACK][KNIGHT] * 3   <  4+3 )
       return 0;
****/
    if( (g.cnt[WHITE][PAWN]  | g.cnt[BLACK][PAWN])==0 )
  {
    if( (g.cnt[WHITE][QUEEN] | g.cnt[BLACK][QUEEN] | g.cnt[WHITE][ROOK ] | g.cnt[BLACK][ROOK])==0  &&
       g.cnt[WHITE][BISHOP] * 4   +   g.cnt[WHITE][KNIGHT] * 3   <  4+3 &&
       g.cnt[BLACK][BISHOP] * 4   +   g.cnt[BLACK][KNIGHT] * 3   <  4+3 )
       //return 0;
       s[WHITE]=0,s[BLACK]=0;
    else{
       if(s[WHITE]==VALUE_K)
       {
          if(g.cnt[BLACK][ROOK] | g.cnt[BLACK][QUEEN] | (g.cnt[BLACK][BISHOP]>1))
             s[BLACK] += VALUE_Q*2;
          else if(s[BLACK]==VALUE_K+VALUE_P){//король перед не фланговой пешкой
              int u=g.kingSq[BLACK]-8;
              if(u>0 && (u&7)!=0 && (u&7)!=7)
               if(g.pos[u]==PAWN)
                  s[BLACK] += VALUE_R;

          }
       }else if(s[BLACK]==VALUE_K){
          if(g.cnt[WHITE][ROOK] | g.cnt[WHITE][QUEEN] | (g.cnt[WHITE][BISHOP]>1))
             s[WHITE] += VALUE_Q*2;
          else if(s[WHITE]==VALUE_K+VALUE_P){//король перед не фланговой пешкой
              int u=g.kingSq[WHITE]+8;
              if(u<64 && (u&7)!=0 && (u&7)!=7)
               if(g.pos[u]==PAWN)
                  s[WHITE] += VALUE_R;
          }
       }

    }
  }

  if(s[g.side] - s[g.xside] + MARGIN <= alpha  ||
     s[g.side] - s[g.xside] - MARGIN >= beta)
     return s[g.side] - s[g.xside];

 // s[0] = s[1] = 0;
//  s[0] /= 10;
//  s[1] /= 10;

  do{
    void GenerateAttackes(void);
    int  Mobile_Pin_XRay(int sq, int p, int c, int* a0, int* a1);
    int KingSFTY(int c);
    int c,i,sq,p,dP,*b_atk0, *b_atk1,c1,hungCnt,score;//,st_score;
    double stage,__stage;

    GenerateAttackes();
    for(c = WHITE; c <= BLACK; c++){
//      int isolCnt = 0;
//      int doubleCnt = 0;
      dP = c==WHITE ? -8:8;
      c1 = c^1;
      b_atk0 = p_attack[c].bit_atk;
      b_atk1 = p_attack[c1].bit_atk;
      hungCnt = 0;
      score = 0;
      //st_score = 0;
      stage = Stage(c);
      __stage = 1.0 - stage;
      for(i = g.start[c]; i <= g.stop[c]; i++)
      {
       if( (sq = g.list[i]) != -1 )
       {
         int b0,b1;

         p = g.pos[sq];
         score += score_table[c][p][sq];

        // score += max(history[c][p][sq]>>3,4);
          /*
         if(g.cnt[WHITE][PAWN] | g.cnt[BLACK][PAWN]){
           if(c==WHITE) st_score += 63-sq;
           else st_score += sq;
         }
         */

         b0 = (b_atk0[sq]&~CNTRL_BISHOP) | ((b_atk0[sq]&CNTRL_BISHOP) << 1);
         b1 = (b_atk1[sq]&~CNTRL_BISHOP) | ((b_atk1[sq]&CNTRL_BISHOP) << 1);


         //АКТИВНОСТЬ В АТАКЕ
         if(b0==0) score -= 1; //не защищена
         else{
            //защищена меньшей или равной
            if(~(comp_piece_cntrl[p] - 1)  &  b0) score += 1;

         }
         if(b1){ //атакована
            score -= b1&15;
            //атакована и не защищена или
            //атакована меньшей частью
            if(b0==0  ||  ((~(comp_piece_cntrl[p] - 1)<<1)  &  b1) ){

              maxHungValue[c] = max(  maxHungValue[c], value[p] );

              score -=  HUNG[p];
              ++hungCnt;
              if(hungCnt==2) score -= 8;
              else if(hungCnt>2) score -= 2;
            }else if( p == PAWN && ((b0 & CNTRL_PAWN)==0) ){
                if( (b1 & 15) > (b0 & 15) )
                  score -= 2; //слабая пешка зависла
                else score -= 1;
            }
          }

          if(p==PAWN){
             int x = COLUMN(sq);
             int half_passed = g.pawn_column_cnt[c1][x]==0;

             //нет пешки-антогониста -> наполовину проходная
             if(half_passed){
               score += HALF_PASSED_PAWN;
               //больше стремится вперед
               if(opponent_board[ROW(sq)]==c) score += 2;
             }


             //нет соседней пешки
             if((b_atk0[sq+dP]&CNTRL_PAWN)==0) score -= 1;

             /*
               штраф за каждую сдвоенную пешку
             */
             if(g.pawn_column_cnt[c][x] > 1){
                // ++doubleCnt;
                 score -= DOUBLE_PAWN;//-1+doubleCnt;
             }

             /*
               штраф за изолированную пешку в зависимости от
               колонки. Центральные пешки получают больший штраф.
               Если изолированная пешка заблокированя - штраф
             */
             if( (x==0 || g.pawn_column_cnt[c][x-1]==0) &&
                 (x==7 || g.pawn_column_cnt[c][x+1]==0) ) {
                // score -= ISOL_PAWN;
                // score -= ISOL_PAWN_COLUMN[x];
               // score -= ISOL_PAWN[ ++ isolCnt];
                score -= ISOL_PAWN;
                if(g.color[sq + dP]==(c1)) score -= 2; //BLOCKED
             }else{ // checked backward pawn
                /*
                  штраф для отсталой пешки
                  если отсталая пешка заблокированя - штраф
                  если отсталая пешка является пешкой-кандидатом, т,е.
                  перед ней нет пешки-антогониста, то дополнительный штраф
                */

                int u = sq + dP;
                while( (unsigned)u < 64 ){
                  if(b_atk0[u] &  CNTRL_PAWN) goto notBackPawn;
                  u -= dP;
                }
                score -= BACK_PAWN;
                if(half_passed) score -= 2; //back. half passed pawn
                if(g.color[sq + dP]==(c1)) score -= 2; //BLOCKED
            notBackPawn:
               score += 0;
             }

             /*
               Если пешка не защищена другой пешкой и не
               находится на стартовой позиции или не имеет соседней
               пешки, то такая пешкасчитается слабой и дается штраф
               за каждую атаку оппонента
             */
             if( (b_atk0[sq] & CNTRL_PAWN)==0  &&
                 (!PAWN_START_POS(sq,c) || (b_atk0[sq+dP] & CNTRL_PAWN)==0 ) )
             {

                score -= (b1 & 15) * BAD_PAWN_ATK; // counter of attacks
             }
             /*
               квадрат перед любой пешкой занят - подвижность такой пешки
               равна 0
             */
             if(g.color[sq+dP] != NEUTRAL){
                 score -= 1; //blocked
                 if(c==WHITE)
                 {
                   if( sq==D2 || sq==E2 ) score -= 2;
                 }else{
                   if( sq==D7 || sq==E7 ) score -= 2;
                 }
             }

             /*
               Проходные пешки
               если впереди по ходу нет пешки оппонента и
               ни одно поле по ходу не атаковано пешкой оппонента,
               то такая пешка считается проходной и премируется.
               Штраф дается за каждую атаку оппонента по ходу пешки
               и за каждую фигуру оппонента, блокирующую движение
               пешки вперед.
               Если проходная пешка имеет пешку-соседа или защищена
               пешкой, то такая пешка считается защищенной проходной
               и премируется дополнительно
             */
             do{
               int u = sq + dP;
               int op_blocked = 0;
               while( (unsigned)u < 64 ){
                 if(b_atk1[u] &  CNTRL_PAWN) goto notPassedPawn;
                 if(g.pos[u]==PAWN && g.color[u]==c1) goto notPassedPawn;
                 if(g.color[u]==c1) op_blocked++;  //opponent pieces blocked
                 op_blocked += b_atk1[u] & 15; //count. attacks
                 u += dP;
               }
               score += PASSED_PAWN + PASSED_PAWN_ROW[c][ROW(sq)] -
                       min(op_blocked,4);
               if(g.color[sq+dP] == c1) score -= 2;
               // protected  passed pawn -> is very good!
               if( (b_atk0[sq] & CNTRL_PAWN)  ||
                   (b_atk0[sq+dP] & CNTRL_PAWN) )
                   score += PASSED_PAWN_ROW[c][ROW(sq)]/2 + 2;
              //end game
              if(stage==0  &&  g.cnt[c1][KNIGHT]+g.cnt[c1][BISHOP]+g.cnt[c1][ROOK]==0)
              {

                 int destSq = c==WHITE? COLUMN(sq) :  8*7 + COLUMN(sq);
                 if( Distance(sq,destSq) < Distance(destSq, g.kingSq[c1]) ){ //квадрат пешки - 100% проходит!!!
                    score += PASSED_PAWN_ROW[c][ROW(sq)] << 3; //мало
                 }else{
                   int rookPawn = (COLUMN(sq)==0) | (COLUMN(sq)==7);//ладейная
                   //король стремится контролировать поле перед пешкой
                   if( b_atk0[sq+dP] & CNTRL_KING ){
                  // if( (g.pos[sq+dP]==KING)  &  (g.color[sq+dP]==c) ){
                       score += 2;
                       //если это не половине оппонента - пешка проходит(если не ладейная)
                       if(!rookPawn && opponent_board[ROW(sq)]==c ) //на полвине противника
                         score += 8;
                   }
                   //для ладейной пешки король оппонента стремится занять
                   //угол
                   if( rookPawn ){
                     //score -= 7-Distance(destSq, g.kingSq[c1]);
                     if(  g.kingSq[c1] == destSq  )  score -= 4;
                   }
                 }
              }

           notPassedPawn:
              score += 0;
             }while(0);


           }else if(p==KING){

              score += KingSFTY(c);

           }else{// other pieces
               //форпост
               if( (b0 & CNTRL_PAWN) && opponent_board[ROW(sq)]==c){
                  int notPawnAtk,u;
                   score += 1;
                   if(g.pawn_column_cnt[c][COLUMN(sq)]==0) score += 1;
                   //квадрат не может быть атакован пешкой противника
                   notPawnAtk = 1;
                   for(u = sq; (unsigned)u < 64; u+=dP)
                    if(b_atk1[u] & CNTRL_PAWN){
                       notPawnAtk = 0;
                       break;
                    }
                   if(notPawnAtk){
                     score += 2;
                   }
               }
               if(p==ROOK){

                 score += ROOK_BONUS;
                 score += Mobile_Pin_XRay(sq,p,c,b_atk0,b_atk1);
                 if(g.pawn_column_cnt[c][COLUMN(sq)]==0){
                    score += 2;  //open line
                    if(g.pawn_column_cnt[c^1][COLUMN(sq)]==0)
                      score += 2; //operation line
                 }
                 if(c==WHITE){
                   if(ROW(sq)==7) score += 1;
                   else if( ROW(sq)==1) score += 4;
                 }else{
                   if(ROW(sq)==0) score += 1;
                   else if( ROW(sq)==6) score += 4;
                 }
                 if(b0 & CNTRL_ROOK) score += 2;
                 //активность и ценность ладьи возрастает с
                 //убыванием материала у противника
                // score +=  ((14-Taxi(sq, g.kingSq[c1])) + 16 + 20)
                //            * __stage;
                 score += 10 * __stage;
                 if( g.cnt[c][ROOK] > 1 ) score += 4;


               }else if(p==BISHOP){

                 score += BISHOP_BONUS;
                 score += Mobile_Pin_XRay(sq,p,c,b_atk0,b_atk1);
                 if(g.cnt[c][BISHOP]>1) score += 8;
                 score += main_diag[sq];

                 if(g.mtl[c^1]==VALUE_K){ //король противника в угол цвета слона
                   static unsigned char black_sq[64]={
                     0,1,0,1,0,1,0,1,
                     1,0,1,0,1,0,1,0,
                     0,1,0,1,0,1,0,1,
                     1,0,1,0,1,0,1,0,
                     0,1,0,1,0,1,0,1,
                     1,0,1,0,1,0,1,0,
                     0,1,0,1,0,1,0,1,
                     1,0,1,0,1,0,1,0,
                   };
                   if(black_sq[sq]){
                     score -= Taxi(56,g.kingSq[c1])<<1;
                     //score -= Taxi(7,g.kingSq[c1]);
                     score += 14;
                   }else{
                     //score -= Taxi(63,g.kingSq[c1]);
                     score -= Taxi(0,g.kingSq[c1])<<1;
                     score += 14;
                   }

                 }

               }else if(p==QUEEN){
                 int cnt;
                 score += QUEEN_BONUS;

                 score += Mobile_Pin_XRay(sq,p,c,b_atk0,b_atk1);
                 if(b0 & CNTRL_ROOK) score += 1;
                 if(b_atk0[sq] & CNTRL_BISHOP) score += 1;
                 if(Distance(sq, g.kingSq[c]) > 2)  score -= 2;

               //  score += (14 - Taxi(sq,g.kingSq[c1])) *
                // (0.5 + StageKnightBishop(c) * 0.7);
                // ;
                 if(Distance(sq, g.kingSq[c1]) <= 3){
                    score += 5;//10*(0.5 + (1.0-StageKnightBishop(c)) * 0.7);
                 }

                 //score += 14 - Taxi(sq, g.kingSq[c1]);
                 cnt = g.cnt[c][ROOK] + g.cnt[c][BISHOP] + g.cnt[c][KNIGHT];
                 if( cnt > 0 ){
                   score += 2;
                   if( cnt > 1 ) score += 4;
                 }
               }else if(p==KNIGHT){
                  score += KNIGHT_BONUS;
                  if(b_atk0[sq] & CNTRL_KNIGHT) score += 1;
                  if( Distance(sq, g.kingSq[c1]) <= 3 )
                    score += 4;
                  score -=  8-(g.cnt[WHITE][PAWN] + g.cnt[BLACK][PAWN])/2;
               }
           }
       }//end if sq != -1
      }
       //ценность пешек возрастает
       score += (g.cnt[c][PAWN]) * __stage;
       if(g.cnt[c][PAWN]==0) score -= 16;
       s[c] += score;
       //s[c] += st_score/8;
     }//for c


  }while(0);


  return s[g.side] - s[g.xside];

}


void  GenScoreTable(int p, int tbl[64], int c, int max, int op_king_sq){


   if(p != PAWN){
      int tmp[64],j,cnt,d,u,n1,max_val,sq;

      for(j = 0; j < 64; j++){
        int row = c==BLACK? ROW(j) : (7 - ROW(j));
        tbl[j] = row;
      //  if(p != KING)
      //    tbl[j] += 14 - Taxi(j,op_king_sq);
        tmp[j] = 0;
      }
      max_val = 0;
      for(cnt = 0; cnt < 3; cnt++){
        for(sq = 0; sq < 64; sq++)
          for(d = d_start[p]; d <= d_stop[p]; d++)
            for(n1 = MAP(sq)+ dir[d]; (n1&0x88)==0; n1 += dir[d]){
              u = UNMAP(n1);
              tmp[sq] += tbl[u];
              if(tmp[sq] > max_val) max_val = tmp[sq];
              if(!SWEEP(p)) break;
            }
        //copy
        for(j = 0; j < 64; j++)
          tbl[j] = tmp[j];
      }
      //zoom result
      for(j = 0; j < 64; j++)
        tbl[j] = (unsigned)tbl[j] * max / max_val;

   }else{ //PAWN
    int CntAtkPromoteSq(int sq, int c);
    int PromotePathLen(int sq, int c);
    double temp[64], max_val = 0;
    int j;

    for(j = 0; j < 64; j++)
      if(ROW(j) != 0  &&  ROW(j) != 7){
        temp[j] = (double)CntAtkPromoteSq(j,c) /
                  PromotePathLen(j,c);
        if(temp[j] > max_val) max_val = temp[j];
      }else temp[j] = 0;
    for(j = 0; j < 64; j++)
      tbl[j] = temp[j] * max / max_val;
   }

}


int CntAtkPromoteSq(int sq, int c){
 int x1,y1,x2,y2,dY;
  //left
  x1 = COLUMN(sq); y1 = ROW(sq);
  dY = c==WHITE? -1:1;
  do{
    x1 -= 1; y1 += dY;
  }while(x1 >= 0 && x1 <= 7 && y1 >= 0 && y1 <= 7);

  //right
  x2 = COLUMN(sq); y2 = ROW(sq);
  dY = c==WHITE? -1:1;
  do{
    x2 += 1; y2 += dY;
  }while(x2 >= 0 && x2 <= 7 && y2 >= 0 && y2 <= 7);

  return x2 - x1 + 1;

}

int PromotePathLen(int sq, int c){
  if(c==WHITE) return ROW(sq);
  else return 7 - ROW(sq);
}

void PrintArray(FILE*f,int tbl[64], int p, int c){
  char *chp[] = {"nopiece","pawn","knight","bishop",
                 "rook","queen","king"};
  char *chc[] = {"white","black"};
  int x,y;

  fprintf(f, "%s %s\n", chc[c], chp[p]);

  for(y = 0; y < 8; y++){
    for(x = 0; x < 8; x++){
      int j = y*8+x;

      fprintf(f,"%3d,",tbl[j]);
    }
    fprintf(f,"\n");
  }


}


void GenerateAttackes(void){
  int c0,i,sq,p,cntrl,j,*bit_atk,dP,n0,n1,u,d,p1;
   
    do{
       const int *rtmp = (int*)p_attack + 128;
       register unsigned long long *rt = (unsigned long long*)p_attack, 
                                   *rs = (unsigned long long*)rtmp;

      rt--; while(++rt < rs) *rt = (unsigned long long)0;
    }while(0);
   // memset(p_attack,0,sizeof(struct Atk_Type)*2);
    for(c0 = WHITE; c0 <= BLACK; c0++){
      //c1 = c0 ^ 1;
      dP = c0==WHITE? -8:8;
      bit_atk = p_attack[c0].bit_atk;
      for(i = g.start[c0]; i <= g.stop[c0]; i++)
        if(  (sq = g.list[i]) != -1 ){
          p = g.pos[sq];
          cntrl = piece_cntrl[p];
          n0 = MAP(sq);
          if(p==PAWN){
             n1 = n0 + dP + dP - 1;
             if( (n1&0x88)==0 ){
               u = UNMAP(n1);
               bit_atk[u] = ++bit_atk[u] | cntrl;
             }
             n1 += 2;
             if( (n1&0x88)==0 ){
               u = UNMAP(n1);
               bit_atk[u] = ++bit_atk[u] | cntrl;
             }
          }else if(SWEEP(p)){//other pieces
             extern int atk[16*16];//,dir_atk[16*16];
             extern const int center;

             for(j = d_start[p]; j <= d_stop[p]; j++){
                d = dir[j];
                for(n1 = n0+d; (n1&0x88)==0; n1 += d){
                  u = UNMAP(n1);
                  bit_atk[u] = ++bit_atk[u] | cntrl;
                  p1 = g.pos[u];
                  if(p1==0) continue;
                  if( SWEEP(p1) && g.color[u]==c0 && (atk[n1-n0+center] & (1<<p1))){
                      for(n1 = n1+d; (n1&0x88)==0; n1 += d){
                         u = UNMAP(n1);
                         ++bit_atk[u];
                         if(g.pos[u]) break;
                      }
                  } //enif
                  break; //out for n1
                }//for n1
             }//for j
          }else{//KNIGHT, KING

             for(j = d_start[p]; j <= d_stop[p]; j++)
               if( ((n1 = n0+dir[j]) & 0x88) == 0 ){
                  u = UNMAP(n1);
                  bit_atk[u] = ++bit_atk[u] | cntrl;
               }
          }

        }
    }//for c
}



/*
  Безопасность короля
*/

int KingSFTY(int c){
   int sq = g.kingSq[c], c1 = c^1, cntCheck, n0 = MAP(sq),
       *a1 = p_attack[c1].bit_atk, *a0 = p_attack[c].bit_atk,
       dP = c==WHITE? -8:8, n1,d,u,j,score=0,cntMoveKing,
       cntPawnScreen,findPawn,
       *hasOpponent = g.cnt[c1],
       *pDir,*pStop;
       
   double stage = Stage(c);

   if(stage==0) return 0;


   /*
      сколько легальных ходов короля
      (кол-во смежных свободных неатакованных клеток)

      +
        кол-во атак в клетки, смежные с королем
   */
  cntMoveKing = 0;
  findPawn = 0;
  cntPawnScreen = 0;

  pDir =  (int*)&dir[d_start[KING]];
  pStop = pDir + 8;
  do{
     n1 = n0+(*pDir);
     if( (n1&0x88) == 0 ){
      int cntAtk;
      u = UNMAP(n1);

      //if( (a1[u] & 15)==0 && g.color[u]!=c)
      //    cntMoveKing++;


      cntAtk = a1[u] & 15;
      if( cntAtk ) score -= cntAtk;
      else if(g.color[u]!=c) cntMoveKing++;
      if(g.pos[u] == PAWN && g.color[u] == c)
      {
        const int pscr[] = {4,6,2,0,0,2,6,4};

        findPawn = 1;

        if(c == WHITE){
          if(ROW(u) <  ROW(sq))
          {
             cntPawnScreen++;
             score += pscr[COLUMN(u)];
          }
        }else{
          if(ROW(u) > ROW(sq))
          {
             cntPawnScreen++;
             score += pscr[COLUMN(u)];
          }
        }
      }// pos[u]==PAWN


     }// n1 & 0x88
     pDir++;
  }while(   pDir <= pStop  );

  if(!findPawn) score -= 4;
  if(cntPawnScreen==0) score -= 8;
  if(g.pawn_column_cnt[c][COLUMN(sq)]==0) score -= 4;




   /*
      Подсчитаем количество  клеток, с которых
      противник может обьявить шах королю.

   */
   cntCheck = 0;

 if(hasOpponent[PAWN])
  if( (unsigned)(sq + dP) < 64 )
  {
   //если квадрат перед королем атакован пешкой  :))
   //неприятеля, то она может обьявить шах следующим ходом!
   if(  a1[ sq + dP]  & CNTRL_PAWN )
   {
     if( COLUMN(sq) > 0 )
     {
        u = sq + dP - 1;
        if( g.color[u] == NEUTRAL  &&
            g.color[u + dP] == c1  &&
            g.pos[u + dP]   == PAWN ) cntCheck++;
     }
     if( COLUMN(sq) < 7 )
     {
        u = sq + dP + 1;
        if( g.color[u] == NEUTRAL  &&
            g.color[u + dP] == c1  &&
            g.pos[u + dP]   == PAWN ) cntCheck++;
     }
   }

   //пешка атакует короля - взятие с шахом!
   if( COLUMN(sq) > 0 )
   {
     u = sq + dP - 1;
     if( a1[u] & CNTRL_PAWN )
       if( g.color[u] == c ) cntCheck++;

   }
   if(COLUMN(sq) < 7 )
   {
     u = sq + dP + 1;
     if( a1[u] & CNTRL_PAWN )
       if( g.color[u] == c ) cntCheck++;

   }
  }


   if(hasOpponent[BISHOP] | hasOpponent[QUEEN])
    for(j = d_start[BISHOP]; j <= d_stop[BISHOP]; j++){
     d = dir[j];
     for(n1 = n0+d; (n1&0x88)==0; n1+=d){
       u = UNMAP(n1);
       if(g.color[u] != c1){
         if(a1[u] & (CNTRL_BISHOP|CNTRL_QUEEN)) cntCheck++;
       }
       if(g.color[u] != NEUTRAL) break;
     }
    }

   if(hasOpponent[ROOK] | hasOpponent[QUEEN])
    for(j = d_start[ROOK]; j <= d_stop[ROOK]; j++){
     d = dir[j];
     for(n1 = n0+d; (n1&0x88)==0; n1+=d){
       u = UNMAP(n1);
       if(g.color[u] != c1){
         if(a1[u] & (CNTRL_ROOK|CNTRL_QUEEN)) cntCheck++;
       }
       if(g.color[u] != NEUTRAL) break;
     }
    }

  if(hasOpponent[KNIGHT])
    for(j = d_start[KNIGHT]; j <= d_stop[KNIGHT]; j++){
     if( ((n1 = n0+dir[j])&0x88)==0 ){
       u = UNMAP(n1);
       if(g.color[u] != c1)
         if(a1[u] & CNTRL_KNIGHT){
             cntCheck++;
             if((cntMoveKing==0) & hasOpponent[QUEEN]) score -= 2; //угроза спертого мата
         }
     }
    }


   switch(cntMoveKing){
     case 0:
       score -= 4;
       if(cntCheck) score -= 8;
       if(a1[u] & 15) score -= 16; //MATE :))
       break;
     case 1:
       score -= 2;
       break;
     case 2:
       score -= 1;
       break;
   }
   score -= min(cntCheck*4, 32);




   //штраф за ход короля до рокировки
   if(g.cnt[c1][QUEEN] && !g.casling[c]){
     int LeftCastlEnable( int c );
     int RightCastlEnable( int c );

     int l = LeftCastlEnable(c);
     int r = RightCastlEnable(c);
     if(l==0) score -= 2;
     if(r==0) score -= 4;
     if(l + r == 0) score -= 12;
   }
   if(g.casling[c]){
     if( COLUMN(sq) > 4 ) // o-o
         score += 8*stage;
     else                 // o-o-o
         score += 6*stage;
   }
   return (stage*score);
}


/*
  Мобильность, а также связка и вскрытая атака
*/

int  Mobile_Pin_XRay(int sq, int p, int c, int* a0, int* a1){
   int score = 0, mobile = 0, n0 = MAP(sq),n1,u,sq1,j,d;


   for(j = d_start[p]; j <= d_stop[p]; j++){
     d = dir[j];
     for(n1 = n0+d; (n1&0x88)==0; n1+=d){
        u = UNMAP(n1);
        if(g.pos[u]==0){
            mobile++;
            continue;
        }else{
           if(g.pos[u]==PAWN || g.pos[u]==KING) break;
           sq1 = u;
           for(n1 = n1+d; (n1&0x88)==0; n1+=d){
              u = UNMAP(n1);
              if(g.pos[u]==0) continue;
              if(g.color[u]== (c^1)){
                  if(comp_value[g.pos[u]] > comp_value[p]  ||
                     a1[u]==0){

                     if(g.color[sq1]==c){
                        score += XRAY;
                        if(g.pos[u]==KING) score += 2;
                     }else{
                        score += PIN;
                        if(g.pos[u]==KING){
                            score += 4; //мертвая связка
                            if(a0[sq1] & CNTRL_PAWN) score += 4;//связанная фигура пропала
                        }
                     }
                  }
              }
              break;
           }//for n1
           break;
        } //if not empty square
      }//for n1
   }//for j


   if(mobile==0) score -= 3;
   //if(p==QUEEN) return score + mobile/6;
   //else
   return score + mobile/3;
}
