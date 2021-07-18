#include <string.h>
//#include <values.h>
#include "chess.h"

/* Таблица самообучения Может содержать как узлы из дебютного справочника (необсчитанные), так и узлы,
добавленные в результате поиска. В функции поиска таблица используется аналогично обычному хеше - ищутся узлы и их оценки.
Вместо поиска может быть сделан библиотечный ход. Находятся все ходы, ведущие к библиотечным
узлам и вычисляется приоритет для каждого хода. По результатам игры может производится самообучение -
повышаться приоритет для выигравшей стороны и понижаться для проигравшей. */

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
  p->learn = 16;
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
  }

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
  Node * node = SearchNode( g.side, g.key );

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

/* список всех табличных ходов из данного узла */
int ListLibMoves( int treeLow, int treeHigh, Node * v[], Move moves[], int * n )
{
  Node * p;
  int j;
  char epd[1024];

  * n = 0;
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



/* ход из библиотеки - случайный выбор по приоритету */
Move MoveFromLib( int treeLow, int treeHigh )
{
  Node * v[512]; //lib. nodes
  Move moves[512], mv = 0;
  int n, add;

  if ( ListLibMoves( treeLow, treeHigh, v, moves, & n ) )
  {

    //найдем минимальный приоритет
    do
    {
      int min = INT_MAX;
      int j;
      for ( j = 0; j < n; j++ )
        if ( v[j]->isFromLib && v[j]->learn < min ) min = v[j]->learn;
      add = 16 - min;
    }
    while ( 0 );

    //выберем ход
    do
    {
      U64 Rand64( void );
      int j, max = -1, tmp;
      for ( j = 0; j < n; j++ )
        if ( v[j]->isFromLib && ( tmp = Rand64() % ( v[j]->learn + add ) ) > max )
        {
          max = tmp;
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
  FILE *f = 0;
  f = fopen( fname, "rb" );
  if ( f )
  {
    Node * p;
    char * buf;
    int add_size, tmp;
    int f_size;
    int readCnt = 0;
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
    fclose( f );
    return 1;
  error:
    fclose( f );
  }
  return 0;


}


void SaveSearchResult( int score, int depth )
{
  Node * p = learn[( int )g.key & ( SZ - 1 )];
  char epd[256];

  MakeEpd( epd );
  //поиск существующего узла
  while ( p )
  {
    if ( p->key == g.key && p->c == g.side && strcmp( epd, p->epd ) == 0 )
    {
      if(depth>=p->depth)
      {
         p->depth = depth;
         p->score = score;
      }
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
        if ( p->depth >= depth )
        {
          * ret_score = p->score;
          return 1;
        }
        else if ( p->isFromLib )
        {
          int margin = 16;
          //узел из дебютной книги, но еще не обсчитан
          //предполагаемая оценка - 0
          if ( 0 + margin <= alpha )
          {
            * ret_score = alpha;
            return 1;
          }else if ( 0 - margin >= beta )
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

/* Самообучение: если оценка увеличивается несколько ходов (для
одной стороны), то увеличивается приоритет ходов, ведущих в эту позицию */
void TryLearn( void )
{
  const int N = 6;
  Node * v[MAX_GAME + MAX_PLY];
  void MakeHistoryScore( Node * v[] );
  int FunctionUp( Node * v[], int n, int c );
  void IncLearnPV( Node * v[], int c, int inc );

  if ( g.isLearn == 0 )
  {
    MakeHistoryScore( v );
    if ( FunctionUp( v, N, g.side ) )
    {
      IncLearnPV( v, g.side, 1 );
      //IncLearnPV( v, g.xside, -1 );
      g.isLearn = 1;
    }
    else if ( FunctionUp( v, N, g.xside ) )
    {
     // IncLearnPV( v, g.xside, 1 );
      IncLearnPV( v, g.side, -1 );
      g.isLearn = 1;
    }

  }
}


void MakeHistoryScore( Node * v[] )
{
  int j, c;

  c = g.side;
  for ( j = g.game_cnt - 1; j >= 0; j-- )
  {
    v[j] = SearchNode( c, g.key_list[j] );
    c ^= 1;
  }
}

/* Функция возрастает, если X(n) > X(n - D) abs(X(n) - X(n-1)) < V

D - 6 V - 24 */


int FunctionUp( Node * v[], int n, int c )
{
  const int D = 6;
  const int V = 24;
  int k = 0, last_score, j;

  if ( c == g.side ) j = g.game_cnt-1;
  else
    j = g.game_cnt - 2;

  for ( ; j >= 0; j -= 2 )
  {
    Node * p = v[j];
    if ( p && p->c == c && p->depth > 2 )
    {
      int up = 1;
      if ( j - D < 0 ) up = 0;
      else if ( v[j - D] == NULL ) up = 0;
      else if ( v[j - D]->depth < 2 ) up = 0;
      else if ( v[j - D]->score >=  p->score ) up = 0;
      else if ( k > 0 &&  !(p->score - V < last_score) ) up = 0;

      if ( up == 0 ) return 0;
      last_score = p->score;
      k++;
      if ( k == n )
      {

        return 1;
      }
    }
    else
      return 0;
  }

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

  if(c==g.side) j = g.game_cnt-1;
  else j = g.game_cnt-2;
  for ( ; j >= 0; j-=2 )
    if( (p = v[j])==NULL  ||  p->c != c) return;


  if(c==g.side) j = g.game_cnt-1;
  else j = g.game_cnt-2;
  for ( ; j >= 0; j-=2 )
      v[j]->learn += inc;


}
