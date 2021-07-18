#include "chess.h"
#include "pvtree.h"


extern int Check( void );
extern int __foo;
extern int root_white_mtl;
extern int root_white_st_score;
extern Root_Score root_list[400];
extern int Use_Null, Null_Margin, Null_Min_Depth;
extern int s_depth, doNull,doReduction;
extern int history[2] [8] [64], mate_history[2] [8] [64];
extern int histMaxVal[2];
extern int histCutVal[2];
extern const int MAX_HIST;
extern const int MIN_HIST;
extern int root_mtl[2];
extern Line pv_line, killer, pv_killer;
extern time_t start_time, curr_time, limit_time;
extern int is_stop_search;
extern U64 cnt_nodes;
extern PVTree *pv_tree;
extern void MakeRootList( void );
extern void RootPick( int low, int high );
extern int Search( int alpha, int beta, int depth, int check,
            Line ret_pv_line, int lazy , int isTree, int useNull);

int XFindMove(Move mv,Move v[],int n){
  int j; 
  for(j=0;j<n;j++)
    if(v[j]==mv) return j;
  return -1;
}

//возврвщает ход ret->mv, и оценку *ret_score
//если есть ход, должна быть оценка, если нет - может быть оценка мата-пата (не 0)
//если есть список исключения, то хода может не быть, т.к. других ходов нет
//может быть превышен буфер игры - этого надо избегать
int XSearchMove( Root_Score *ret, Move exclMoves[], int exclN, int *ret_score , int beta_target)
{
  void HistoryInit( void );
  void TimeReset( void );
  int StopSearch( void );
  const int MAX_DEPTH = MAX_PLY - 10;
  int d;
  void HashClear( void );
  void InitEvaluate( void );
  time_t s_time = limit_time;
  int old_score,Xcnt;
  int MakeRootListWithExcludeMoves(Move exclMoves[],int exclN );

  memset( ret, 9, sizeof( Root_Score ) );
  ret->mv=0;
  if(ret_score)*ret_score=0;

  //инициализация переменных для поиска
  root_white_mtl = g.mtl[WHITE] - g.mtl[BLACK];
  root_white_st_score = g.st_score[WHITE] - g.st_score[BLACK];//0x002
  TimeReset();
  srand( time( 0 ) );
  InitEvaluate();
  HashClear();
  HistoryInit();
  memset( killer, 0, sizeof( killer ) );
  memset( pv_killer, 0, sizeof( pv_killer ) );

  //список легальный ходов без исключений
  LegalMoveList();
  Xcnt=treeCnt[1];    //сколько легальных ходов до вычеркивания
  if(MakeRootListWithExcludeMoves(exclMoves,exclN)!=Xcnt-exclN)
      assert(0);

  if ( treeCnt[1] == treeCnt[0] ){         //нет ходов
      if(exclN<=0)                         //не было исключений
      {
        if(Check()){
           if(ret_score) *ret_score = -INF;     //MATE
        }
        else {
           if(ret_score) *ret_score=-1;                //STALEMATE
        }
      }
      return 0;
  }

  //формируем вспомогательное дерево для функции поиска
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


  root_mtl[WHITE] = g.mtl[WHITE];
  root_mtl[BLACK] = g.mtl[BLACK];
  Use_Null = g.cnt[WHITE][PAWN] + g.cnt[BLACK][PAWN] > 0  &&
             abs(g.mtl[WHITE] + g.mtl[BLACK]) > 2*VALUE_Q;
  memset(pv_line,0,sizeof(Line));
  s_time = limit_time;       //!, чтото устаревшее
 // if( g.game_cnt < 30*2 )
 //   limit_time *= 2;


  for ( d = 2; d <= MAX_DEPTH && !StopSearch(); d += 1 )
  {
    int tmp;
    void MakeHistCutValue(void);

    MakeHistCutValue();


    s_depth = d;
    tmp = Search( -INF, beta_target, s_depth, Check(), pv_line, 0, 1,0 );
    if(!is_stop_search){
       if(ret_score)*ret_score = tmp;
       ret->mv  = pv_tree->root->mv;
    }
    if ( tmp > INF - 100 && s_depth > 6) break;
    if( tmp < -INF + 100 && s_depth > 6 ) break;
    if ( ( curr_time - start_time ) * 4.9 > limit_time ) break;

    if( s_depth > 2  &&
       abs(old_score - tmp) < 80  &&
       (curr_time - start_time) * 4.9 > s_time ) break;
    old_score = tmp;
  }
  //limit_time = s_time;

done:

  if ( treeCnt[1] == treeCnt[0] ) return 0;
 
  if(exclN<=0)
  {
    void SaveSearchResult( int score, int depth );
    void TryLearn( void );
    SaveSearchResult( root_list[0].score, root_list[0].depth );
    TryLearn();
  }

  return 1;
}
///////////////////
int XTest( Root_Score *ret,int MaxMoves){
    Move mv[10];
    int score[10];
    int n=0,j;
    Root_Score tmp;
    char *MoveToStr( Move mv, char * tr );
    char buf[32];
    int beta_target=INF;


    while(n<MaxMoves && n<10 ){
     XSearchMove( &tmp, mv, n, &score[n],beta_target );
     mv[n]=tmp.mv;
     if(mv[n]==0 && score[n]==0) break;
     beta_target=score[n];
     n++;
     if(mv[n-1]==0) break;
    }
    for(j=0;j<n;j++){
        printf("analis: %s - %d \n", MoveToStr(mv[j],buf),score[j]);
    }


}
