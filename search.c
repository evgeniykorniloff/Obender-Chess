#include "chess.h"
#include "pvtree.h"
#include "time.h"
#include "stdlib.h"
//#define DEBUG

/* typedef Move Line[MAX_PLY]; enum{ EMPTY,ALPHA,EXACT,BETA }; typedef struct{ int score,flag,depth; Line line; }CS;
typedef struct{ CS s[2]; Move mv; }Root_Score; */
int Check( void );
extern int __foo;
int root_white_mtl;
int root_white_st_score;
Root_Score root_list[400];
int Use_Null = 1, Null_Margin = 0, Null_Min_Depth = 4;
int s_depth, doNull,doReduction;
int history[2] [8] [64], mate_history[2] [8] [64];
int histMaxVal[2];
int histCutVal[2];
const int MAX_HIST = 1000000;
const int MIN_HIST = 1000;
int root_mtl[2];

extern time_t start_time, curr_time, limit_time;
extern int is_stop_search;
extern U64 cnt_nodes;
  static PVTree *pv_tree;


typedef struct
{
  int phase, low, high, sort, * alpha, moveFromTree, isTree;
  Move hash_mv, pv_mv, kill_mv, pvkill_pv;
  int cut,cutCnt;
  PVNode *list_pv;
  int cntBestKillers;

}
PhaseInfo;

void PhaseInfoReset( PhaseInfo * p, int * alpha )
{
  p->phase = p->low = p->high = p->sort = p->moveFromTree = p->isTree = 0;
  p->hash_mv = p->pv_mv = p->kill_mv = p->pvkill_pv = 0;
  p->list_pv = 0;
  p->alpha = alpha;
  p->cut = p->cutCnt = p->cntBestKillers = 0;
}

Line pv_line, killer, pv_killer;

#define PV (vtr.moveFromTree && legalCnt <= 3)
#define MAIN_PV (vtr.moveFromTree && legalCnt ==1 && alpha==-INF && beta==INF)

#define MTL(c)  (g.mtl[c] - g.mtl[c^1])
#define ROOT_MTL(c)  (root_mtl[c] - root_mtl[c^1])

//0x001
static int __cntFindNodeInLearnTbl;
int Search( int alpha, int beta, int depth, int check,
            Line ret_pv_line, int lazy , int isTree, int useNull)
{
  int Quies( int alpha, int beta );
  int StopSearch( void );
  int Evaluate( int alpha, int beta );
  int HashLook( int alpha, int beta, int depth, int * ret_score, Move * ret_mv );
  void HashInsert( int alpha, int beta, int depth, Move mv );
  int NextMove( PhaseInfo * p, Move * mv );
  int Check( void );
  void SaveHist( Move mv, int alpha, int beta );
  void SavePvLine( Line dest, Line source, Move mv );
  Move LastMove( void );
  int Repetition( void );
  int LegalCastl( Move mv );
  void SaveRootScore( int alpha, int beta, Move mv, int depth, Line line );
  int LearnLook( int depth, int alpha, int beta, int * ret_score );
  int score;
  Move mv = 0, best_mv = 0;
  PhaseInfo vtr;
  Line tmp_line;
  int c0 = g.side, c1 = c0 ^ 1;
  //int old_alpha = alpha;
  int L_MTL = MTL(c0);

  if( check ){
    if(depth <= 0) depth = 1;

  }else if( ply > s_depth * 3 )
    depth = 0;

  PhaseInfoReset( & vtr, & alpha );
  vtr.isTree = isTree;

  if ( depth <= 0 ) return Quies( alpha, beta );
  else if ( StopSearch() ) return 0;
  else if ( ply > MAX_PLY - 4 ) return Evaluate( alpha, beta );
  else if ( ply > 0 && Repetition() ) return 0;
  else if ( ply > 4 && s_depth>6 /*&& !doNull*/ && LearnLook( depth, alpha, beta, & score ) ){
      __cntFindNodeInLearnTbl++;
      return score;
  }
  else if ( HashLook( alpha, beta, depth, & score, &vtr.hash_mv ) &&
            !check
          ) 
      return score;
  else
  {
    int legalCnt = 0;
    int cntGoodMoves;
    Move last = LastMove();
    int mateThreat = 0;
    int oneReply = 0;

    //STATIC SEARCH  AND  NULL CUT //0x001
    if(depth <= 2  && ply > 0 && ( doNull | check)== 0  && (last&PROMOTE)==0)
    {

       //if( Evaluate(beta-1,beta) >= beta)
       if((g.mtl[g.side] - g.mtl[g.xside]) - (VALUE_N-500) > beta)
          return beta;
    }


    if(  Use_Null && !lazy && !check && ply > Null_Min_Depth)
    do{
      int saveDoNull = doNull,tmp;
       
      //good capture in momentum ? not null move :))  0x001
      
      if(depth > 2  && (last & (CAPTURE|PROMOTE)) )
        //if((last&CAPTURE) && (g.tag_list[ply-1].cap_p==PAWN||(last&PROMOTE))) //слабая пешка или превращение

      {
        const int N = 5;// 11;
        int good_cap = 0;
        int P = g.game_cnt + ply;
        int mtl = g.mtl[WHITE] - g.mtl[BLACK];

         //??
         //   printf("--->\n"); 

        if(g.xside==WHITE)
        {
            if(P-N <= 0)
               good_cap = mtl > root_white_mtl && abs(mtl) < VALUE_R;
            else
               good_cap = mtl > g.white_mtl_list[P-N] && abs(mtl) < VALUE_Q;
        }else{
            if(P-N <= 0)
               good_cap = mtl < root_white_mtl && abs(mtl) < VALUE_R;
            else
               good_cap = mtl < g.white_mtl_list[P-N] && abs(mtl) < VALUE_Q;
        }

      //  if(good_cap && (g.mtl[g.side] - g.mtl[g.xside]) - 2*VALUE_P > beta)
      //    good_cap = 0;
        if(good_cap){
         // printf("#----> iid\n");
          break;
        }
      }
       


        doNull = 1;
        treeCnt[ply + 1] = treeCnt[ply];
        MakeMove( 0 );
        tmp_line[ply] = 0;
        tmp = -Search( -(beta), -(beta-1), depth-3, 0, tmp_line, 1, 0, 0 );
        UnMakeMove( 0 );
        doNull = saveDoNull;
        if(tmp >= beta) return beta;
       if(tmp < beta  &&  beta > -INF+100 && tmp < -INF+100)
          mateThreat = 1;

}while(0);


if(ply > 0)
{

 if(check){
    int LegalMoveList(void);
    if ( LegalMoveList() == 0 ) return INF - ply; //CAP. OP. KING
    else if ( treeCnt[ply + 1] == treeCnt[ply] ) return -INF + ply + 1; // mate
    else if ( treeCnt[ply + 1] == treeCnt[ply] + 1 ) oneReply = 1;
 }else{

    if ( Generate() == 0 ) return INF - ply; //CAP. OP. KING
    else if ( treeCnt[ply + 1] == treeCnt[ply] ) return 0; // ?? not realy
 }

}else if ( treeCnt[ply + 1] == treeCnt[ply] ) return 0; // ?? not realy

    cntGoodMoves = max((treeCnt[ply + 1] - treeCnt[ply])/3, 3);


    while ( NextMove( & vtr, & mv ) )
    {
      int nextCheck = 0, nextDepth = depth - 1, cutt_off = 0;
      int add = 0, threat = 0,nextUseNull, prob_score;

      if ( ( mv & ( LEFT_CASTL | RIGHT_CASTL ) ) && !LegalCastl( mv ) )
        continue;
      MakeMove( mv );
      if ( SqAttack( g.kingSq[g.xside], g.side ) )
      { // illegal move
        UnMakeMove( mv );
        continue;
      }
      legalCnt++;


      nextCheck = Check();


      //extension in search
      if ( (ply > 2 || (nextCheck && nextDepth==0))
             &&
           (ply < 20 || ply< (s_depth<<1) || (nextCheck && nextDepth==0)) )
      {
      //////////////ext
        if ( nextCheck ){
           add += 1;
           threat = 1;
        }
        if ( PIECE( mv ) == PAWN && ( ROW( TO( mv ) ) == ( c0 == WHITE ? 1 : 6 ) ) )
        {
           add += 1;
           threat = 1;
        }

        if(mateThreat ){

           add += 1;
           threat = 1;
        }
        if(oneReply ){

            add += 1;
        }
        ////// 0x001
      //recaprure  in momentum ?  :))
      if(s_depth > 4  && (mv & (CAPTURE|PROMOTE)) && add==0 && ply>2 && ply<s_depth)
      {

        const int N = 5,N1=2;
        int good_recap = 0;
        int P = g.game_cnt + ply;
        int mtl = g.mtl[WHITE] - g.mtl[BLACK];

        if(P-N>0 && mtl == g.white_mtl_list[P-N])good_recap=1;
        else if(P-N1>0 && mtl == g.white_mtl_list[P-N1])good_recap=1;
        if(good_recap){
           add += 1;
            //printf("# -----> iid \n");

        }
       /*************
        const int N = 5;// 11;
        int good_recap = 0;
        int P = g.game_cnt + ply;
        int mtl = g.mtl[WHITE] - g.mtl[BLACK];
        if(P-N <= 0){
          good_recap = mtl==root_white_mtl && abs(mtl) < VALUE_R;;
        }else{
            good_recap = mtl == g.white_mtl_list[P-N] && abs(mtl) < VALUE_Q;
        }
        if(good_recap &&
          (g.mtl[g.xside] - g.mtl[g.side])  //side is change in makemove
             + 2*VALUE_P < alpha)
          good_recap = 0;
        if(good_recap){
           add += 1;
            //printf("# -----> iid \n");

        }
        *********/
      }



        ///////



        if(add > 1) add = 1;
        nextDepth  = nextDepth + add;


        if(add == 0 && check && !doNull && depth <= 2 && ply<(s_depth<<1))
        {
           if( oneReply ) nextDepth += 1;
           else if( treeCnt[ply+1] - treeCnt[ply] == 2 ) ;
           else {
              nextDepth--;
              if(nextCheck && nextDepth<=0) nextDepth++;
           }
        }

        ///////////ext
       }
  if(nextCheck && nextDepth<=0) nextDepth++;
  nextUseNull = 1;//legalCnt > 3 || nextDepth<=2;
  if(MAIN_PV || nextDepth <= 0){// || legalCnt<=3){

       tmp_line[ply] = 0;
       score = -Search( -beta, -alpha, nextDepth, nextCheck, tmp_line,0, vtr.moveFromTree,
         nextUseNull);
      // printf("#----->deb\n");

  }else{
    int iid_enable = //0;  //0x002

                      ply > 4    &&
                      //!doNull  &&
                      Use_Null &&
                      s_depth > 6  &&
                      depth > 2    &&
                      legalCnt > 3 &&
                      nextDepth<depth &&
                      (mv&(CAPTURE|PROMOTE))==0 &&
                      (g.mtl[g.xside] - g.mtl[g.side]) + VALUE_P/2 < alpha;

             
      //good simple move in momentum ?  :))  0x002
      if(iid_enable)
      {
        const int N = 6;// 11;
        int good_move = 0;
        int P = g.game_cnt + ply;
        int st_score = g.st_score[WHITE] - g.st_score[BLACK];

        if(g.xside==WHITE)
        {
            if(P-N <= 0)
               good_move = st_score > root_white_st_score;
            else
               good_move = st_score > g.white_st_score_list[P-N];
        }else{
            if(P-N <= 0)
               good_move = st_score < root_white_st_score;
            else
               good_move = st_score < g.white_st_score_list[P-N];
        }
        if(good_move)
        {
           iid_enable=0;
        }

      }


     
    // if(ply<=4)iid_enable=1;


     if( iid_enable )
     {
       int save = doReduction;
       doReduction = 1;
    //??
    //printf("# -----> iid \n");
       tmp_line[ply] = 0;
       prob_score = -Search( -(alpha+1), -alpha, nextDepth-1, nextCheck, tmp_line,0, vtr.moveFromTree,
                    nextUseNull || nextDepth-1<=2);
       doReduction = save;

       if(prob_score <= alpha){
          score = alpha;
          //if(ply>4)
          goto skipSearch;
       }


     }else{
        if(legalCnt <= 3) prob_score = alpha + 1;
        else prob_score = alpha;
     }

     if( prob_score > alpha ){
       tmp_line[ply] = 0;
       score = -Search( -beta, -alpha, nextDepth, nextCheck, tmp_line,0,vtr.moveFromTree,
                       nextUseNull );
     }else{
        tmp_line[ply] = 0;
        score = -Search( -(alpha+1), -alpha, nextDepth, nextCheck, tmp_line,0,vtr.moveFromTree,
                        nextUseNull);
        if(score > alpha && score < beta){// || legalCnt <= cntGoodMoves){
          tmp_line[ply] = 0;
         score = -Search( -beta, -alpha, nextDepth, nextCheck, tmp_line,0,vtr.moveFromTree,
                          nextUseNull);
       }
     }

   }

skipSearch:
      UnMakeMove( mv );

      if ( is_stop_search ) return 0;

      if ( !cutt_off && score > alpha )
      {
        best_mv = mv;
        alpha = score;
        if ( TO( mv ) != TO( last ) ) killer[ply] = mv;
        SaveHist( mv, alpha, beta );
        if ( ply == 0 )
          SaveRootScore( alpha, beta, mv, depth, tmp_line );


        if ( alpha < beta )
        {
          SavePvLine( ret_pv_line, tmp_line, mv );
          pv_killer[ply] = mv;
        }

        ////////  save node (pv line) in tree //////////////
        if( isTree && (doNull | doReduction )==0 ){
           //save node

           PVNode **p = pvFindNodeTree(pv_tree, &g.game_list[g.game_cnt], ply);
           if( p ){

             PVNode **fn = pvFindMove(p, mv);
             if( fn ) pvToFront(fn, p);
             else pvInsertToFront(pv_tree, p,
                                  pvNewNode(pv_tree,depth,g.side,alpha,nextCheck, mv,g.key));
           }

           //save line

           if(ply == 0  &&  alpha < beta ){
              int j;
              PVNode **p = &pv_tree->root;


              for( j = 0; j < 80 &&  ret_pv_line[j]; j++){
                 PVNode **fn = pvFindMove(p, ret_pv_line[j]);
                 MakeMove( ret_pv_line[j] );
                 if( fn ){
                    PVNode **tmp = pvToFront(fn, p);
                    if(tmp && *tmp){
                        (*tmp)->depth = depth - j;

                    }
                 }else{
                    pvInsertToFront(pv_tree, p,
                                    pvNewNode(pv_tree,depth-j,g.side,alpha,Check(), ret_pv_line[j],g.key));
                 }
                 p = &(*p)->nextPly;
              }
              for( j = j - 1; j >= 0; j-- ){
                 UnMakeMove( ret_pv_line[j] );
              }
           }

        };
        ////////  end save node (pv line) in tree //////////////


        if(alpha >= beta)
          break;
      }
    } //while

    if ( legalCnt == 0 )
    {
      int *h,incH;  
      if ( !check ) return 0; //STALEMATE
      //save mate piece position
      h = &history[g.xside] [PIECE( LastMove() )] [TO( LastMove() )];
      incH = 5 + rand()&7;
      if(*h + incH < MAX_HIST) *h += incH;
      else *h = MAX_HIST;

      h = &mate_history[g.xside][PIECE( LastMove() )][TO( LastMove() )];
      *h = min(MAX_HIST,*h+1);
      return -INF + ply + 1; //MATE
    }

  }
  if ( ply > 0 )
    HashInsert( alpha, beta, depth, best_mv );

  return alpha;
}



int Quies( int alpha, int beta )
{
  int GenerateCaptures( void );
  void QSort( int low, int high );
  void Pick( int low, int high );

  if ( StopSearch() ) return 0;
  else
  {
    int score = Evaluate( alpha, beta );
    if ( score > alpha ) alpha = score;
    if ( score >= beta ) return alpha;
    else if ( ply > MAX_PLY - 4 ) return alpha;
    else if ( ply > 0 && GenerateCaptures() == 0 ) return INF - ply;
    else if ( treeCnt[ply + 1] == treeCnt[ply] ) return alpha;
    else
    {
      int sort = 0;
      int j;
      int maxCapValue = -INF;
      Move last = LastMove();
      for ( j = treeCnt[ply]; j < treeCnt[ply + 1]; j++ )
      {
        Move mv = tree[j];
        int s = 0;
        if ( mv & CAPTURE )
        {
          if ( mv & EN_PASSANT )
          {
            s += 2 * VALUE_P;
          }
          else
          {
            int cap_v = value[g.pos[TO( mv )]];
            s += cap_v - PIECE( mv );
            if ( TO( mv ) == TO( last ) ) s += VALUE_R;
            if( cap_v > maxCapValue )
              maxCapValue = cap_v;
          }
        }
        if ( mv & PROMOTE ) s += value[PIECE( mv )];
        sort_val[j] = s;
      }


      if ( treeCnt[ply + 1] - treeCnt[ply] > 20 &&
           g.mtl[g.side] - g.mtl[g.xside] + maxCapValue < alpha
          )
      {
        QSort( treeCnt[ply], treeCnt[ply + 1] - 1 );
        sort = 1;
      }


      for ( j = treeCnt[ply]; j < treeCnt[ply + 1]; j++ )
      {
        Move mv;
        int tmp;

        if ( !sort ) Pick( j, treeCnt[ply + 1] - 1 );
        mv = tree[j];
        MakeMove( mv );
        tmp = -Quies( -beta, -alpha );
        UnMakeMove( mv );
        if ( tmp > alpha ) alpha = tmp;
        if ( alpha >= beta ) break;
      } //for
    }
  }
  return alpha;
}





void SaveHist( Move mv, int alpha, int beta )
{
  int incH = 1+rand()&1;
  int *h = &history[g.side] [PIECE( mv )] [TO( mv )];
  if ( alpha < beta ) incH += 1;
  if(alpha > VALUE_P*2) incH += 2;
 // if(alpha > VALUE_P*2) incH += 1;
  if ( alpha > INF - 100 ){
      incH += 5 + rand()&7;
      //??1
      //printf("#------>\n");
      int *h1 = &mate_history[g.side][PIECE( mv )][TO( mv )];
      if(*h1<MAX_HIST) (*h1)++;
  }
 // if( *h < MIN_HIST ) *h = MIN_HIST;
  if ( *h + incH < MAX_HIST ) ( *h ) += incH;
  else{
    int j,k,n;
    for(j=0;j<2;j++)for(k=0;k<8;k++)for(n=0;n<64;n++)
        history[j][k][n] /= 2;
    *h += incH;
  }
  histMaxVal[g.side] = min( histMaxVal[g.side] + incH, MAX_HIST);
}


int Repetition( void )
{
  int ci = g.game_cnt + ply - 1;

  //  return 0;


  if ( ci >= 4 )
  {
    Move * mv = & g.game_list[ci];
    HashKey * key = & g.key_list[ci];
    int cnt = 0, rep = 0;
    while ( mv >= g.game_list )
    {
      if ( cnt > 0 && * key == g.key && ( ++rep == 2 || ci >= g.game_cnt - 1 ) ) return 1;
      if ( ( * mv & ( CAPTURE | PROMOTE | LEFT_CASTL | RIGHT_CASTL ) ) || * mv == 0 || PIECE( * mv ) == PAWN )
        return 0;
      cnt++; ci--;
      if ( cnt >= 50 ) return 1;
      mv--; key--;
    }
  }
  return 0;
}

int NextMove( PhaseInfo * q, Move * ret_mv )
{
  void QSort( int low, int high );
  void Pick( int low, int high );
  void RootPick( int low, int high );
  int k, j;
  Move last = LastMove();

  switch ( q->phase )
  {
    case 0:
      {
        last = LastMove();
        /*
        q->low = treeCnt[ply];
        q->high = treeCnt[ply + 1];


        if ( ply == 0 )
        {
          q->phase = 4;
          goto ply0;
        }
         */
        q->low = treeCnt[ply];
        q->high = treeCnt[ply + 1];

        q->phase++;
      }

   case 1:{

       if( q->isTree && (doNull | doReduction )==0 ){
           //save node
         // if( ply < 2 )
         // {
            PVNode **tmp  = pvFindNodeTree(pv_tree, &g.game_list[g.game_cnt], ply);
            if(tmp && *tmp) q->list_pv = *tmp;
         // }else
         //   q->list_pv = pvFindPosWithHash(pv_tree, g.key, g.side);
       }
       q->phase++;
   }

   case 2:{
      //Pick all pv-moves
      if( q->list_pv ){
        if ( q->low < q->high ){
          int j;
          for( j = q->low; j < q->high; j++)
           if( tree[j] == q->list_pv->mv ){
               q->list_pv = q->list_pv->moves;
               if( j != q->low ){
                 SWAP( sort_val[q->low], sort_val[j] );
                 SWAP( tree[q->low], tree[j] );
               }
               * ret_mv = tree[q->low++];
               q->moveFromTree = 1;
               return 1;
           }

        }
      }
      q->moveFromTree = 0;
      q->phase++;
   }

   case 3:{


        if ( doNull == 0 )
        {
          q->pv_mv = pv_line[ply]; //pv_line[ply] = 0;
        }
        q->kill_mv = killer[ply];
        q->pvkill_pv = pv_killer[ply];
        // PICK ALL CAPTURES
        for ( k = j = q->low; j < q->high; j++ )
        {
          Move mv = tree[j];
          int s = 0;
          if ( mv & CAPTURE )
          {
            if ( mv & EN_PASSANT )
            {
              s += 2 * VALUE_P;
            }
            else
            {
              //extern int max(int v1,int v2);
              s += value[g.pos[TO( mv )]] - PIECE( mv );
              //s += score_table[g.xside][g.pos[TO(mv)]] [TO(mv)];
              if ( TO( mv ) == TO( last ) ) s += VALUE_R;

            }
          }
          if ( mv & PROMOTE ) s += value[PIECE( mv )];
          //if ( mv == q->pv_mv ) s += 4 * VALUE_Q;
          if ( mv == q->hash_mv ) s += 2 * VALUE_Q;
          if ( mv == q->kill_mv ) s += 2 * VALUE_P - 10;
          if ( mv == q->pvkill_pv ) s += VALUE_P - 10;
          //if(  mv == q->pv_mv ) s += 30;


          if ( s > 0 )
          {
//            s += history[g.side][PIECE(mv)][TO(mv)];

            tree[j] = tree[k];
            tree[k] = mv;
            sort_val[k] = s;
            k++;
          }
        } //for
        q->high = k;
        q->sort = 0;
        q->phase++;
        if ( q->high - q->low > 30 )
        {
          QSort( q->low, q->high - 1 );
          q->sort = 1;
        }
      } //case 0

    case 4:
      {
        if ( q->low < q->high )
        {
          if ( q->sort == 0 ) Pick( q->low, q->high - 1 );
          * ret_mv = tree[q->low++];
          return 1;
        }
        q->phase++;

      } //case 1
    case 5:
      {
        int j,k;
        int cntHist;
        q->low = q->high;
        q->high = treeCnt[ply + 1];

        for ( k = j = q->low; j < q->high; j++ )
        {
          int t;
          Move mv = tree[j];
          t = history[g.side] [PIECE( mv )] [TO( mv )];
          if( t > 0 ){
            if( j != k )
            {
              Move tmp = tree[k];
              tree[k]  = mv;
              tree[j]  = tmp;
            }
            sort_val[k] = t;
            k++;
          }
        }
        q->high = k;
        cntHist = q->high - q->low;
        q->phase++;
        q->sort = 0;
        q->cutCnt = q->low +  cntHist/2;
        q->cntBestKillers =  q->cutCnt;
       } //case

    case 6:
      {
        if ( q->low < q->high )
        {

          if( q->low == q->cntBestKillers &&
              q->high - q->low > 25){

              QSort( q->low, q->high - 1 );
              q->sort = 1;
          }

          if ( q->sort == 0 ) Pick( q->low, q->high - 1 );
          q->cut = q->low >= q->cutCnt;
          * ret_mv = tree[q->low++];
          return 1;
        }

      } //case 3
      q->phase++;
      q->low = q->high;
      q->high = treeCnt[ply + 1];
      q->cut = 1;

    case 7:{

        if ( q->low < q->high )
        {
          * ret_mv = tree[q->low++];
          return 1;
        }

        return 0;
    } //case 7

    }//switch
  return 0;
}


void Pick( int low, int high )
{
  int m_i = low, max = sort_val[low], j;
  for ( j = low + 1; j <= high; j++ )
    if ( sort_val[j] > max )
    {
      max = sort_val[j];
      m_i = j;
    }

  if ( m_i != low )
  {
    SWAP( sort_val[low], sort_val[m_i] );
    SWAP( tree[low], tree[m_i] );
  }
}



void SavePvLine( Line dest, Line source, Move mv )
{
  int j;

  for ( j = ply + 1; j < MAX_PLY - 4 && source[j]; j++ )
    dest[j] = source[j];

  dest[ply] = mv;
  dest[j] = 0;
}


void QSort( int low, int high )
{
  int i, j, t;

  if ( low < high )
  {
    t = sort_val[( unsigned )( low + high ) / 2];
    i = low; j = high;
    do
    {
      while ( sort_val[i] < t ) i++;
      while ( t < sort_val[j] ) j--;
      if ( i <= j )
      {
        SWAP( sort_val[i], sort_val[j] );
        SWAP( tree[i], tree[j] );
        i++; j--;
      }
    }
    while ( i < j );
    QSort( low, j );
    QSort( i, high );
  }
}


void SaveRootScore( int alpha, int beta, Move mv, int depth, Line line )
{
  void PrintSearchStatus( int ply, int score, int time, U64 nodes, Move * pv );
  Root_Score * p, * stop;



  p = root_list + treeCnt[0];
  stop = root_list + treeCnt[1];
  for ( ; p < stop; p++ )
    if ( p->mv == mv )
    {
      int j;

      p->depth = depth;
      p->score = alpha;
      p->flag = alpha < beta ? EXACT : BETA;
      memset( p->line, 0, sizeof( Line ) );
      p->line[0] = mv;
      for ( j = 1; j < MAX_PLY - 4 && line[j]; j++ )
        p->line[j] = line[j];
      PrintSearchStatus( p->depth, p->score, ( int )( curr_time - start_time ), cnt_nodes, p->line );
      //   }

      return;
    }
  assert( 0 );
}

void MakeRootList( void )
{
  int j;
  memset( root_list, 0, sizeof( root_list ) );
  LegalMoveList();

  for ( j = 0; j < treeCnt[1]; j++ )
  {
    Root_Score * p = & root_list[j];

    p->mv = tree[j];

    MakeMove( tree[j] );

    p->depth = 0;
    p->flag = EXACT;
    p->score = -Evaluate( -INF, INF );

    UnMakeMove( tree[j] );
  }
}

int CompRootNodes( Root_Score * p1, Root_Score * p2 )
{



  if ( p1->depth > p2->depth )
    return 1;
  if ( p1->depth == p2->depth && p1->score > p2->score )
    return 1;
  return 0;
}


void RootPick( int low, int high )
{
  Root_Score max = root_list[low];
  int m_i = low;
  int j;


  for ( j = low + 1; j <= high; j++ )
    if ( CompRootNodes( & root_list[j], & max ) )
    {
      max = root_list[j];
      m_i = j;
    }

  if ( m_i != low )
  {
    root_list[m_i] = root_list[low];
    root_list[low] = max;
    SWAP( tree[low], tree[m_i] );
    SWAP( sort_val[low], sort_val[m_i] );
  }
}

int SweepCnt(int c){

  return g.cnt[c][QUEEN] + g.cnt[c][ROOK] + g.cnt[c][BISHOP];
}


//функция должна оставить при выходе treeCnt[0] = 0 !!!!

int SearchMove( Root_Score * ret )
{
  void HistoryInit( void );
  void TimeReset( void );
  int StopSearch( void );
  const int MAX_DEPTH = MAX_PLY - 10;
  int d;
 // Line tmp_line;
  void HashClear( void );
  void InitEvaluate( void );
  Move MoveFromLib( int treeLow, int treeHigh );
  time_t s_time = limit_time;
  int old_score;

  //инициализация переменных для поиска
  __cntFindNodeInLearnTbl = 0;
  root_white_mtl = g.mtl[WHITE] - g.mtl[BLACK];
  root_white_st_score = g.st_score[WHITE] - g.st_score[BLACK];//0x002
  TimeReset();
  srand( time( 0 ) );
  InitEvaluate();
  HashClear();
  HistoryInit();
  memset( killer, 0, sizeof( killer ) );
  memset( pv_killer, 0, sizeof( pv_killer ) );
  MakeRootList();
  if ( treeCnt[1] == treeCnt[0] ) goto done;

  do{
     int j;
     if( pv_tree == NULL ) pv_tree = malloc(sizeof(PVTree));


     pvReset( pv_tree );
     for( j = 0; j < treeCnt[1]; j++)
     {
       HashKey key = g.key;
       int color   = g.side;
       RootPick(j, treeCnt[1]-1);
       MakeMove(  tree[j] );
       pvInsertToFront(pv_tree, &pv_tree->root ,
                                  pvNewNode(pv_tree,0,color,0,Check(),
                                  tree[j],key));
       UnMakeMove(  tree[j] );

     }

  }while(0);
  //SEARCH

  //попробуем найти ход из дебютного справочника
  do
  {
    Move mv;
    int N1, N2;
    //чем дальше от начала игры, тем меньше
    //вероятность использования библиотечного хода
    N1 = 20;
    N2 = g.game_cnt;
    if ( N2 > 12 ) N2 = 12;


    if ( rand() % ( N1 - N2 ) != 1 )
      if ( ( mv = MoveFromLib( treeCnt[0], treeCnt[1] - 1 ) ) != 0 )
      {
        memset( ret, 0, sizeof( Root_Score ) );
        ret->mv = mv;
        return 1;
      }
  }
  while ( 0 );

  root_mtl[WHITE] = g.mtl[WHITE];
  root_mtl[BLACK] = g.mtl[BLACK];
  Use_Null = g.cnt[WHITE][PAWN] + g.cnt[BLACK][PAWN] > 0  &&
             abs(g.mtl[WHITE] + g.mtl[BLACK]) > 2*VALUE_Q;
  memset(pv_line,0,sizeof(Line));
  s_time = limit_time;
  if( g.game_cnt < 30*2 )
    limit_time *= 2;
  for ( d = 2; d <= MAX_DEPTH && !StopSearch(); d += 1 )
  {
    int tmp;
    void MakeHistCutValue(void);

//    if( d == 7 ) continue;


    MakeHistCutValue();
#ifdef DEBUG
  printf(" histCutVal: %10d %10d\n", histCutVal[0],histCutVal[1]);
#endif

    s_depth = d;
    tmp = Search( -INF, INF, s_depth, Check(), pv_line, 0, 1,0 );
    if ( tmp > INF - 100 ) break;
    if( tmp < -INF + 100 && s_depth > 6 ) break;
    if ( ( curr_time - start_time ) * 2 > limit_time ) break;

    if( s_depth > 2  &&
       abs(old_score - tmp) < 80  &&
       (curr_time - start_time) * 2 > s_time ) break;
    old_score = tmp;
  }
  limit_time = s_time;

done:

  if ( treeCnt[1] == treeCnt[0] ) return 0;
  //RootPick( 0, treeCnt[1] - 1 );
  // * ret = root_list[0];
  memset( ret, 9, sizeof( Root_Score ) );
  ret->mv  = pv_tree->root->mv;
  /*
  do
  {
    int HashDamp( void );
    printf( "#hash used: %d\%  \n", HashDamp() );
  }
  while ( 0 );
  */


  do
  {
    void SaveSearchResult( int score, int depth );
    void TryLearn( void );

    SaveSearchResult( root_list[0].score, root_list[0].depth );
    TryLearn();
  }
  while ( 0 );

  ///?????
  /*
  do{
    FILE *f = fopen("test", "w");
    if(f){
      fprintf(f,"-------------------\n");

      pvPrint(pv_tree->root , f);
      fclose(f);
    }
  }while(0);
    */
  /////?????
  //??
  //printf("#__cntFindNodeInLearnTbl=%d\n",__cntFindNodeInLearnTbl);
  return 1;
}



void MakeHistCutValue(void){
  int c,sq,p;
  int tmp[2][8][64];
  U64 nodes[2] = {0,0};

  for(c = WHITE; c <= BLACK; c++)
   for(p = PAWN; p <= KING; p++)
    for(sq = 0; sq < 64; sq++)
    {
      nodes[c] += history[c][p][sq];
      tmp[c][p][sq] = history[c][p][sq];
    }

  for(c = WHITE; c <= BLACK; c++)
  {
    U64 cnt = 0;
    histCutVal[c] = 0;
    while( cnt < nodes[c]/2 )
    {
     int minVal = INT_MAX-1, *ptr = 0;
     for(p = PAWN; p <= KING; p++)
       for(sq = 0; sq < 64; sq++)
       {
         if( tmp[c][p][sq] > 0 && tmp[c][p][sq] < minVal )
         {
           minVal = tmp[c][p][sq];
           ptr = &tmp[c][p][sq];
         }
       }
      if( ptr == 0 ) break;
      cnt += (*ptr);
      histCutVal[c] = max( histCutVal[c], *ptr );
      *ptr = 0;
    }//while
  }
}

void HistoryClear(void){
   histMaxVal[WHITE] = histMaxVal[BLACK] = 0;
   histCutVal[WHITE] = histCutVal[BLACK] = 0;
   memset(history,0,sizeof(history));
   memset(mate_history,0,sizeof(history));   
}
void HistoryInit( void )
{
    
  //if(g.game_cnt%40<3){
  //   HistoryClear();
  //   exit;
  //}
  histMaxVal[WHITE] = histMaxVal[BLACK] = 0;
  histCutVal[WHITE] = histCutVal[BLACK] = 0;
  do
  {
    int c, p, sq;
    for ( c = WHITE; c <= BLACK; c++ )
      for ( p = PAWN; p <= KING; p++ )
        for ( sq = 0; sq < 64; sq++ )
        { 
          int mh=mate_history[c][p][sq];
          history[c][p][sq]=mh>0?rand()%(mh/2+1):0;
        }
  }while(0);
   /*
  //RESET HISTORY
  do
  {
    int c, p, sq;
    for ( c = WHITE; c <= BLACK; c++ )
      for ( p = PAWN; p <= KING; p++ )
        for ( sq = 0; sq < 64; sq++ )
        {

          int score = score_table[c] [p] [sq], * h = & history[c] [p] [sq];

          if ( score < 0 ) * h = -( rand() % ( -score ) );
          else if ( score > 0 ) * h = rand() % score;
          else
            * h = 0;


        }
  }
  while ( 0 );
 */
}
