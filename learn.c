#include <string.h>
//#include <values.h>
#include "chess.h"

/* Таблица самообучения Может содержать как узлы из дебютного справочника (необсчитанные), так и узлы,
добавленные в результате поиска. В функции поиска таблица используется аналогично обычному хеше - ищутся узлы и их оценки.
Вместо поиска может быть сделан библиотечный ход. Находятся все ходы, ведущие к библиотечным
узлам и вычисляется приоритет для каждого хода. По результатам игры может производится самообучение -
повышаться приоритет для выигравшей стороны и понижаться для проигравшей. 
*/

static int LEARN_START_VAL = 16;
typedef struct LearnNode
{
  int score, //оценка позиции (только для обсчитанных узлов - depth > 0)
       learn, //приоритет
       c, //чей ход ожидается
       depth, //глубина поиска (если поиска не было - 0)
       size, //размер структуры (динамический - описание позиции может быть различным)
       isFromLib; //ход из библиотеки 1..0
  struct LearnNode * next; //следующий элемент списка
  HashKey key; //хеш ключ позиции    
  char epd[1]; //..size  строковое представление позиции
}
Node;



#define SZ (1<<16)  //размер таблицы указателей (начала списков)
Node * learn[SZ]; //хеш таблица (начала списков указателей)
/*
void LearnClear(void){
  memset(learn,0,sizeof(learn));

}
*/

void LearnClear(void)
{
  memset(learn,0,sizeof(learn));
}


/* новый элемент, динамический размер */
Node * NewLNode( char * epd )
{
  Node * p = ( Node * ) malloc( sizeof( Node ) + strlen( epd ) );

  p->score = p->depth = 0; p->next = 0;
  p->isFromLib = 1;
  p->learn = LEARN_START_VAL;
  p->key = g.key;
  p->c = g.side;
  p->size = sizeof( Node ) + strlen( epd );
  strcpy( p->epd, epd );
  return p;
}



typedef struct TokType
{
  int hlat[256];
  char * begin, * end;
}
TokType;



void InitToken( TokType * p, char * s, char * del )
{
  int j;
  for ( j = 0; j < 256; j++ ) p->hlat[j] = 0;
  for ( ; * del; del++ ) p->hlat[( unsigned char ) ( * del )] = 1;
  p->begin = p->end = s;
  p->end--;
}

char * GetToken( TokType * p )
{

  if ( * p->begin == '\0' )
    return 0;

  for ( p->begin = p->end + 1; p->hlat[( unsigned char ) ( * p->begin )] == 1 && * p->begin != '\0'; p->begin++ );
  for ( p->end = p->begin; p->hlat[( unsigned char ) ( * p->end )] == 0 && * p->end != '\0'; p->end++ );
  * p->end = '\0';

  if ( * p->begin == '\0' )
    return 0;

  return p->begin;
}


/* читает текстовый файл и добавляет в таблицу */
void ReadLibFile( char * fname )
{
  int GetStr( char * s, int n, FILE * f );
  void InsertNodeLearn( void );
  void InitNewGame( void );
  int StrToMove( char * s, Move * ret_mv );

  FILE * f = fopen( fname, "r" );
  printf("# try load lib file\n");
  if ( f )
  {
    char s[0xFFFF], * strMv;
    Move mv;
    int lines = 0;
    struct Game_Type save = g, new_game;
    TokType t, new_token;
    InitToken( & t, s, " !?#,;\n" );
    InitNewGame();
    new_game = g;
    new_token = t;

    while ( GetStr( s, 0xFFFF - 1, f ) )
    {
      lines++;
      t = new_token;
      g = new_game;
      while ( ( strMv = GetToken( & t ) ) != NULL )
      {
        if ( StrToMove( strMv, & mv ) && InsertMoveInGame( mv ) )
          InsertNodeLearn(); //текущая позиция
        else
        {
          printf( "#error: line %d, lexem %s\n", lines, strMv );
          break; //error
        }
      }
    }
    fclose( f );
    g = save;
  }else
    printf("# lib file not found\n");


}


int GetStr( char * s, int n, FILE * f )
{
  while ( fgets( s, n, f ) )
  {
    char * p = s;
    while ( * p == ' ' ) p++;
    if ( * p != '\0' && * p != '#' && * p != '\n' ) return 1;
  }
  return 0;
}




/* вставляет текущую позицию */
void InsertNodeLearn( void )
{
  Node * SearchNode( int c, HashKey key );
  void MakeEpd( char epd[] );
  char epd[256];

	
  Node *node = SearchNode( g.side, g.key );
  MakeEpd( epd );
	
  if ( node == NULL || strcmp( node->epd, epd ) != 0 )
  {
    Node * p = NewLNode( epd );
    p->next = learn[( int )g.key & ( SZ - 1 )];
    learn[( int )g.key & ( SZ - 1 )] = p;
  }
}

Node * SearchNode( int c, HashKey key )
{

  Node * p = learn[( int )key & ( SZ - 1 )];
  while ( p )
  {
    if ( p->key == key && p->c == c )
      return p;
    p = p->next;
  }
  return NULL;
}

/* считывает текущую позицию в формат epd(pen) r1bqkbnr/ppp1pppp/2n5/3p4/3PP3/8/PPP2PPP/RNBQKBNR w KQkq d3 0 3 */

void MakeEpd( char * s )
{
  const char ch_piece[2] [8] =
  {
    {
      0, 'P', 'N', 'B', 'R', 'Q', 'K', 0
    },
    {
      0, 'p', 'n', 'b', 'r', 'q', 'k', 0
    }
  };

  //считываем позицию
  do
  {
    int x, y, cnt;
    for ( y = 0; y < 8; y++ )
    {
      x = 0;
      while ( x < 8 )
      {
        cnt = 0;
        while ( x < 8 && g.pos[y * 8 + x] == NOPIECE )
        {
          x++;
          cnt++;
        }
        if ( cnt > 0 ) * s++ = '0' + cnt;
        while ( x < 8 && g.pos[y * 8 + x] != NOPIECE )
        {
          * s++ = ch_piece[g.color[y * 8 + x]] [g.pos[y * 8 + x]];
          x++;
        }
      }
      if ( y < 7 ) * s++ = '/';
    }
  }
  while ( 0 );


  //чей ход ожидается
  * s++ = ' ';
  if ( g.side == WHITE ) * s++ = 'w';
  else
    * s++ = 'b';
  * s++ = ' ';

  //доступность рокировок
  do
  {
    int LeftCastlEnable( int c );
    int RightCastlEnable( int c );
    char * p = s;

    if ( LeftCastlEnable( WHITE ) ) * s++ = 'Q';
    if ( RightCastlEnable( WHITE ) ) * s++ = 'K';
    if ( LeftCastlEnable( BLACK ) ) * s++ = 'q';
    if ( RightCastlEnable( BLACK ) ) * s++ = 'k';

    if ( p == s ) * s++ = '-';
  }
  while ( 0 );
  * s++ = ' ';
  //если последний ход - пешкой через клетку,
  //то обозначим поле для генерации взятия через битое поле

  do
  {
    int PawnFirstExtMove( Move mv, int c );
    extern const char * chb[];
    Move mv = LastMove();
    if ( PawnFirstExtMove( mv, g.xside ) )
    {
      int sq = FROM( mv ) + dP[g.side];
      * s++ = chb[sq] [0];
      * s++ = chb[sq] [1];
    }
    else
      * s++ = '-';
  }
  while ( 0 );


  /* удалим конечные пробелы и прочерки */
  for ( s--; * s == ' ' || * s == '-'; s-- );
  s++;
  * s = '\0';
}

int LeftCastlEnable( int c )
{
  if ( c == WHITE )
  {
    if ( g.pos[E1] == KING && g.color[E1] == WHITE && g.pos[A1] == ROOK && g.color[A1] == WHITE
         && ( g.castl_enable & WHITE_LEFT_CASTL_ENABLE ) )
           return 1;
  }
  else
  {
    if ( g.pos[E8] == KING && g.color[E8] == BLACK && g.pos[A8] == ROOK && g.color[A8] == BLACK
         && ( g.castl_enable & BLACK_LEFT_CASTL_ENABLE ) )
           return 1;
  }
  return 0;
}

int RightCastlEnable( int c )
{
  if ( c == WHITE )
  {
    if ( g.pos[E1] == KING && g.color[E1] == WHITE && g.pos[H1] == ROOK && g.color[H1] == WHITE
         && ( g.castl_enable & WHITE_RIGHT_CASTL_ENABLE ) )
           return 1;
  }
  else
  {
    if ( g.pos[E8] == KING && g.color[E8] == BLACK && g.pos[H8] == ROOK && g.color[H8] == BLACK
         && ( g.castl_enable & BLACK_RIGHT_CASTL_ENABLE ) )
           return 1;
  }
  return 0;
}


int PawnFirstExtMove( Move mv, int c )
{
  if ( PIECE( mv ) == PAWN )
  {
    if (
       ( ROW( FROM( mv ) )   ==  (( c == WHITE ) ? 6 : 1) ) &&
       ( ROW( TO( mv ) )     ==  (( c == WHITE ) ? 4 : 3)
       ) )
      return 1;
  }
  return 0;
}


/* список всех табличных ходов из данного узла 
   в v[] возвращает список позиций
   в moves[] - ходы, ведущие в данные позиции
   *n - кол-во найденных позиций
   на вход:
	   treeLow,treeHigh - начало, конец списка легальных
	   ходов из данной позиции
 */

int ListLibMoves( int treeLow, int treeHigh, Node *v[], Move moves[], int *n )
{
  Node * p;
  int j;
  char epd[1024];

  *n = 0;
  for ( j = treeLow; j <= treeHigh; j++ )
  {
    MakeMove( tree[j] );
    p = learn[( int )g.key & ( SZ - 1 )];
    while ( p )
    {
      if ( p->key == g.key && p->c == g.side )
      {
        MakeEpd( epd );
        if ( strcmp( epd, p->epd ) == 0 )
        {
          v[* n] = p;
          moves[* n] = tree[j];
          ( * n ) ++;
        }
      }
      p = p->next;
    }
    UnMakeMove( tree[j] );
  }
  return ( * n ) > 0;
}



/* ход из библиотеки - случайный выбор по приоритету 
   int treeLow, int treeHigh - номера первого и последнего
   элемента в списке легальных ходов 
*/
Move MoveFromLib( int treeLow, int treeHigh )
{
  Node * v[1024]; //lib. nodes
  Move moves[1024], mv = 0;
  int n;// add;
  srand(time(0));
  if ( ListLibMoves( treeLow, treeHigh, v, moves, & n ) )
  {
    //выберем ход
    //из потомков с минимальным случайным приоритетом
    //0x001
    do
    {
      //U64 Rand64( void );
      int j, min = INT_MAX-1, tmp;
      for ( j = 0; j < n; j++ )
        if ( v[j]            &&
             v[j]->isFromLib && 
           ( tmp = rand() % ( v[j]->learn  ) ) < min 
           )
        {
          min = tmp;
          mv = moves[j];
        }
    }
    while ( 0 );
  }
  return mv;
}


int SaveLearnTable( char * fname )
{
  FILE * f = fopen( fname, "wb" );
  if ( f )
  {
    int j;
    for ( j = 0; j < SZ; j++ )
    {
      Node * p = learn[j];
      while ( p )
      {
        fwrite( p, 1, p->size, f );
        p = p->next;
      }
    }

    fclose( f );
    return 1;
  }
  return 0;
}

/* Читает файл, представляющий собой последовательную запись структур Node, (каждая структура имеет переменный размер)
и заносит каждую структуру в таблицу learn. */
int LoadLearnTable( char * fname )
{
  int readCnt = 0;
  FILE *f = 0;
  f = fopen( fname, "rb" );
  if ( f )
  {
    Node * p;
    char * buf;
    int add_size, tmp;
    int f_size;
    //get file-size
    fseek( f, 0, SEEK_END );
    f_size = ftell( f );
    fseek( f, 0, SEEK_SET );
    //get memory
    buf = malloc( f_size );
    if ( buf == 0 )
      goto error;
    //read file
    do
    {
      tmp = fread( buf, 1, sizeof( Node ), f );
      if ( tmp == 0 ) goto success;
      if ( tmp != sizeof( Node ) )
        goto error;
      p = ( Node * ) & buf[0];
      add_size = p->size - sizeof( Node );
      if ( add_size <= 0 ) goto error;
      if ( fread( buf + sizeof( Node ), 1, add_size, f ) != ( unsigned )add_size )
        goto error;
      buf += p->size;
      readCnt += p->size;
      p->next = learn[( int )p->key & ( SZ - 1 )];
      learn[( int )p->key & ( SZ - 1 )] = p;

    }
    while ( 1 );


  success:
    if(f)fclose( f ),f=NULL;
    if(readCnt==0)
       fprintf(stderr,"#warning:  not load learn file!");
    return  readCnt > 0;

error:
    fprintf(stderr,"#warning:  not load learn file! \n");
    if(f)fclose( f ),f=NULL;
  }
  if(f)fclose( f ),f=NULL;
  return 0;
}


void SaveSearchResult( int score, int depth )
{
  Node * p;
  char epd[256];

  p = learn[( int )g.key & ( SZ - 1 )];
  MakeEpd( epd );
  //поиск существующего узла
  while ( p )
  {
    if ( p->key == g.key && 
         p->c == g.side && 
         strcmp( epd, p->epd ) == 0 )
    {
      p->depth = depth;
      p->score = score;
      return;
    }
    p = p->next;
  }
  //вставляем новый узел
  p = NewLNode( epd );
  p->next = learn[( int )g.key & ( SZ - 1 )];
  learn[( int )g.key & ( SZ - 1 )] = p;

  p->score = score;
  p->learn = 0;
  p->c = g.side;
  p->depth = depth;
  p->isFromLib = 0;
}

/* поиск результата в функции поиска */
int LearnLook( int depth, int alpha, int beta, int * ret_score )
{
  Node * p = learn[( int )g.key & ( SZ - 1 )];
  char epd[256];

  while ( p )
  {
    if ( p->key == g.key && p->c == g.side )
    {
      MakeEpd( epd );
      if ( strcmp( epd, p->epd ) == 0 )
      {
        if ( p->depth >= depth ) //узел обсчитан ранее, можно использовать оценку
        {
          //*ret_score = p->score;
           //вот здесь я позволил себе похулиганить
           // p->score - реальная оценка
           //  + [-3..3] вероятностный бонус из самообучения
          *ret_score = p->score + (LEARN_START_VAL - p->learn)/(LEARN_START_VAL/3);
          return 1;
        }
        else if ( p->isFromLib ) 
        { //0x002
          int margin = LEARN_START_VAL;
          int score = LEARN_START_VAL - p->learn; //p->learn [0..LEARN_START_VAL*2]
                                                  // start = LEARN_START_VAL
          //узел из дебютной книги, но еще не обсчитан
          //предполагаемая оценка - 0
          //изначально score=0, потом корректируется +-LEARN_START_VAL
          if ( score + margin <= alpha )
          {
            * ret_score = alpha;
            return 1;
          }else if ( score - margin >= beta )
          {
            * ret_score = beta;
            return 1;
          }

        }
        return 0;
      }
    }
    p = p->next;
  }

  return 0;
}

/* 
   Самообучение: 
     если оценка растет, 
     то увеличивается приоритет ходов, 
     ведущих в эту позицию  
*/

void TryLearn( void )
{
  const int Len = 10;//за сколько ходов растет
  Node * v[MAX_GAME + MAX_PLY];
  void MakeHistoryScore( Node * v[] );
  int FunctionUp( Node * v[], int n, int c );
  void IncLearnPV( Node * v[], int c, int inc );

  if ( g.isLearn < 10  &&
       (
       ((g.game_cnt&~1)==10  || (g.game_cnt&~1)==20) //чтобы часто не обучалась
           ||
       (g.game_cnt>=4 && g.isLearn==0)//первое обучение
       )
     )
  {
    //0x001
    MakeHistoryScore( v );
    if( FunctionUp( v, Len, g.side ) )
    {
      IncLearnPV( v, g.side, 1 );
      g.isLearn++;
    }
    else if ( FunctionUp( v, Len, g.xside ) )
    {
      IncLearnPV( v, g.xside, 1 );
      g.isLearn++;
    }

  }
}

//0x001
void MakeHistoryScore( Node * v[] )
{
  int j;
  for ( j = g.game_cnt - 1; j >= 0; j-- )
  {
    if((v[j] = SearchNode( WHITE, g.key_list[j] ))==NULL)
        v[j] = SearchNode( BLACK, g.key_list[j] );
  }
}

/* 
   Средняя за период 
здесь проблема в том, что массив v[] может
содержать ходы введенные пользователем для
которых нет Node самоообучения и оценок
соответственно
Т.е. 100% это самообучение работает
корректно когда программа играет сама
с собой и не использует первый ход
из дебютного справочника !!!
*/
//0x001
int SMA(Node *v[],int c, int len, int *result){
  int j;
  int cntFind,k;
  *result=0;
  
  for ( cntFind=0,k=0, j=g.game_cnt-1; 
        j >= 0 && k<len; 
      j -= 1, k++ )
  {
    Node *p = v[j];
    if( p && p->depth>0)//если узел есть и он был обсчитан
    {
       if(p->c==c){
         *result += p->score;
       }else{
	 *result -= p->score;
       }
       cntFind++;
    }
    
  }
  if(cntFind < 2)return 0;  //найдено меньше 2 реальных оценок-какая средняя?
  if(k < len) return 0;//просканировано меньше заданной длины
  if(cntFind < len/2) return 0;//если ход машины-ход игрока, то это условие выполнится
  *result /= cntFind; //средняя оценка
  return 1;
}

//0x001
/*
 Функция растет, если короткое скользящее среднее
 больше 0 и больше длинного скользящего среднего
*/
int FunctionUp( Node * v[], int len, int c )
{
  int sma1,sma2;
  if(SMA(v,c,len/2,&sma1)  &&
     SMA(v,c,len,&sma2) &&
     sma1 > 8 &&  // if sma1 > 0 ?   ;  8 - просто некоторый коридор около 0
     sma1 > sma2
     )
     return 1;
  return 0;
}




/*
Самообучение проводится только если
вся строка осмыслена
*/
void IncLearnPV( Node * v[], int c, int inc )
{
  int j;
  Node * p;
  //0x001
  for(j=g.game_cnt-1; j >= 0; j--)
   if( (p = v[j])!=NULL)
    if(p->c == c)
     {
       //0x002
       if(p->learn+inc<=LEARN_START_VAL*2)
         p->learn+=inc;
       else
         p->learn=LEARN_START_VAL*2;
     }else{
       if(p->learn-inc>=2) //2 оставляем чтобы вариант не совсем пропал
         p->learn-=inc;
       else
         p->learn=2;
     }
}
