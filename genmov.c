#include "chess.h"

int ply;
const int dP[2]      = {  -8, 8},
prom_row[2]          = {  0, 7},
first_pawn_row[2]    = {  6, 1},
d_start[7]           = {  6, 4, 8,  4, 0, 0, 0}, 
d_stop [7]           = {  7, 5, 15, 7, 3, 7, 7},
 dir [16] =
{
  1, 0x10, -1, -0x10, 0x0F, 0x11, -0x0F, -0x11, 
  0x0E, -0x0E, 0x12, -0x12, 0x1F, -0x1F, 0x21, -0x21
};

struct Game_Type g;

int treeCnt[MAX_PLY];
Move tree[MAX_PLY * 100];
int sort_val[MAX_PLY * 100];
int mtl_path[2][MAX_GAME+MAX_PLY];



extern U64 cnt_nodes;
int __foo;

void assert( int expr )
{
  if ( !expr )
  {
    __foo++;
    abort();

  }
}



int Generate( void )
{
  Move * list = & tree[treeCnt[ply]], temp;
  int c0 = g.side, c1 = g.xside, sq, i, p, u, d, n0, n1, j, opKingSq = g.kingSq[c1];
  void MakePromote( Move * * q );


  treeCnt[ply + 1] = treeCnt[ply]; //not moves

  //EN_PASSANT
  do
  {
    int en_pass_to = 0;

    //??
    //  if(cnt_nodes==48094)
    //    __foo++;


    if ( g.game_cnt + ply == 0 ) en_pass_to = g.__en_pass_sq; //init. EPD string
    else
    {
      Move last = g.game_list[g.game_cnt + ply - 1];
      if ( last )
        if ( PIECE( last ) == PAWN )
          if ( abs( ROW( TO( last ) ) - ROW( FROM( last ) ) ) == 2 )
            en_pass_to = TO( last ) + dP[c0];
    }

    if ( en_pass_to )
    {
      int tmp_from = en_pass_to - dP[c0], from;
      if(COLUMN(tmp_from) < 7){
        from = tmp_from  + 1;
        if ( g.pos[from] == PAWN && g.color[from] == c0 )
          *list++ = CAPTURE | EN_PASSANT | ( PAWN << 12 ) | ( from << 6 ) | en_pass_to;
      }
      if(COLUMN(tmp_from) > 0){
        from = tmp_from  - 1;
        if ( g.pos[from] == PAWN && g.color[from] == c0 )
          *list++ = CAPTURE | EN_PASSANT | ( PAWN << 12 ) | ( from << 6 ) | en_pass_to;
      }

    }
  }
  while ( 0 );

  //OTHER MOVES
  for ( i = g.start[c0]; i <= g.stop[c0]; i++ )
    if ( ( sq = g.list[i] ) != -1 )
    {
      p = g.pos[sq];
      temp = ( p << 12 ) | ( sq << 6 );
      if ( p == PAWN )
      {
        int column = COLUMN( sq );
        u = sq + dP[c0];
        if ( g.color[u] == NEUTRAL )
        {
          * list++ = temp | u;
          if ( ROW( u ) == prom_row[c0] ) MakePromote( & list );
          else if ( ROW( sq ) == first_pawn_row[c0] )
          {
            u += dP[c0];
            if ( g.color[u] == NEUTRAL )
              * list++ = temp | u;
          }
        }

        if ( column > 0 && g.color[u = sq + dP[c0] - 1] == c1 )
        {
          if ( u == opKingSq ) return 0;
          * list++ = temp | CAPTURE | u;
          if ( ROW( u ) == prom_row[c0] ) MakePromote( & list );
        }

        if ( column < 7 && g.color[u = sq + dP[c0] + 1] == c1 )
        {
          if ( u == opKingSq ) return 0;
          * list++ = temp | CAPTURE | u;
          if ( ROW( u ) == prom_row[c0] ) MakePromote( & list );
        }

      }
      else if ( SWEEP( p ) )
      {
        n0 = MAP( sq );
        for ( j = d_start[p]; j <= d_stop[p]; j++ )
        {
          d = dir[j];
          for ( n1 = n0 + d; ( n1 & 0x88 ) == 0; n1 += d )
          {
            u = UNMAP( n1 );
            if ( g.color[u] == NEUTRAL ) * list++ = temp | u;
            else
            {
              if ( g.color[u] == c1 )
              {
                if ( u == opKingSq ) return 0;
                * list++ = temp | CAPTURE | u;
              }
              break;
            }
          }
        }
      }
      else
      {
        n0 = MAP( sq );
        for ( j = d_start[p]; j <= d_stop[p]; j++ )
        {
          n1 = n0 + dir[j];
          if ( ( n1 & 0x88 ) == 0 )
          {
            u = UNMAP( n1 );
            if ( g.color[u] == NEUTRAL ) * list++ = temp | u;
            else if ( g.color[u] == c1 )
            {
              if ( u == opKingSq ) return 0;
              * list++ = temp | CAPTURE | u;
            }
          }
        }
      }
    } //for i




  if ( RIGHT_CASTL_ENABLE( c0 ) )
  {
    int sq = g.kingSq[c0];
    if ( ( sq == ( c0 == WHITE ? E1 : E8 ) ) && g.color[sq + 1] == NEUTRAL && g.color[sq + 2] == NEUTRAL
         && g.color[sq + 3] == c0 && g.pos[sq + 3] == ROOK )
           * list++ = RIGHT_CASTL | ( KING << 12 ) | ( sq << 6 ) | ( sq + 2 );
  }


  if ( LEFT_CASTL_ENABLE( c0 ) )
  {
    int sq = g.kingSq[c0];
    if ( ( sq == ( c0 == WHITE ? E1 : E8 ) ) && g.color[sq - 1] == NEUTRAL && g.color[sq - 2] == NEUTRAL
         && g.color[sq - 3] == NEUTRAL && g.color[sq - 4] == c0 && g.pos[sq - 4] == ROOK )
           * list++ = LEFT_CASTL | ( KING << 12 ) | ( sq << 6 ) | ( sq - 2 );
  }


  treeCnt[ply + 1] = list - tree;

  return 1;
}


int GenerateCaptures( void )
{
  Move * list = & tree[treeCnt[ply]], temp;
  int c0 = g.side, c1 = g.xside, sq, i, p, u, d, n0, n1, j, opKingSq = g.kingSq[c1];
  void MakePromote( Move * * q );


  treeCnt[ply + 1] = treeCnt[ply]; //not moves

  //EN_PASSANT

  //OTHER MOVES
  for ( i = g.start[c0]; i <= g.stop[c0]; i++ )
    if ( ( sq = g.list[i] ) != -1 )
    {
      p = g.pos[sq];
      temp = ( p << 12 ) | ( sq << 6 );
      if ( p == PAWN )
      {
        int column = COLUMN( sq );
        u = sq + dP[c0];
        if ( ROW( u ) == prom_row[c0] )
          if ( g.color[u] == NEUTRAL )
          {
            * list++ = temp | u;
            MakePromote( & list );
          }

        if ( column > 0 && g.color[u - 1] == c1 )
        {
          int u1 = u - 1;
          if ( u1 == opKingSq ) return 0;
          * list++ = temp | CAPTURE | u1;
          if ( ROW( u1 ) == prom_row[c0] ) MakePromote( & list );
        }

        if ( column < 7 && g.color[u + 1] == c1 )
        {
          int u1 = u + 1;
          if ( u1 == opKingSq ) return 0;
          * list++ = temp | CAPTURE | u1;
          if ( ROW( u1 ) == prom_row[c0] ) MakePromote( & list );
        }

      }
      else if ( SWEEP( p ) )
      {
        n0 = MAP( sq );
        for ( j = d_start[p]; j <= d_stop[p]; j++ )
        {
          d = dir[j];
          for ( n1 = n0 + d; ( n1 & 0x88 ) == 0; n1 += d )
          {
            u = UNMAP( n1 );
            if ( g.color[u] == NEUTRAL );
            else
            {
              if ( g.color[u] == c1 )
              {
                if ( u == opKingSq ) return 0;
                * list++ = temp | CAPTURE | u;
              }
              break;
            }
          }
        }
      }
      else
      {
        n0 = MAP( sq );
        for ( j = d_start[p]; j <= d_stop[p]; j++ )
        {
          n1 = n0 + dir[j];
          if ( ( n1 & 0x88 ) == 0 )
          {
            u = UNMAP( n1 );
            if ( g.color[u] == c1 )
            {
              if ( u == opKingSq ) return 0;
              * list++ = temp | CAPTURE | u;
            }
          }
        }
      }
    } //for i


  treeCnt[ply + 1] = list - tree;

  return 1;
}






Move LastMove( void )
{
  int j = g.game_cnt + ply - 1;
  if ( j >= 0 ) return g.game_list[j];
  else
    return 0;
}

void MakePromote( Move * * q )
{
  Move * list = ( * q ) - 1, temp = ( ( * list ) & ~( 7 << 12 ) ) | PROMOTE;

  * list++ = temp | ( QUEEN << 12 );
  * list++ = temp | ( KNIGHT << 12 );
  * q = list;
}



void InsertPiece( int p, int c, int sq, int i )
{

  assert( g.pos[sq] == 0 && g.index[sq] == -1 && g.list[i] == -1 );

  g.pos[sq] = p;
  g.color[sq] = c;
  g.index[sq] = i;
  g.list[i] = sq;


  g.mtl[c] += value[p];
  g.cnt[c] [p] ++;
  if ( p == PAWN )
  {
    g.pawn_column_cnt[c] [COLUMN( sq )] ++;
    g.pawn_row_cnt[c] [ROW( sq )] ++;
  }
  else if ( p == KING ) g.kingSq[c] = sq;

  g.key ^= hashRnd[c] [p] [sq];

}


void RemovePiece( int p, int c, int sq, int i )
{

  assert( g.pos[sq] == p && g.color[sq] == c && g.index[sq] == i && g.list[i] == sq );

  g.pos[sq] = NOPIECE;
  g.color[sq] = NEUTRAL;
  g.index[sq] = -1;
  g.list[i] = -1;


  g.mtl[c] -= value[p];
  g.cnt[c] [p] --;
  if ( p == PAWN )
  {
    g.pawn_column_cnt[c] [COLUMN( sq )] --;
    g.pawn_row_cnt[c] [ROW( sq )] --;
  }

  g.key ^= hashRnd[c] [p] [sq];

}


void MakeMove( Move mv )
{
  int cnt;
  cnt = g.game_cnt + ply;
  if ( mv )
  {
    int from = FROM( mv ), to = TO( mv ), p = PIECE( mv ), new_p = p, i, c;
    Tag_Move * tag;

    if ( mv & PROMOTE ) p = PAWN;
    i = g.index[from];
    tag = & g.tag_list[cnt];
    c = g.side;


    tag->castl_enable = g.castl_enable;

    //??
    //  if(cnt_nodes==48094  &&  TO(mv)==31 && FROM(mv)==22)
    //    __foo++;
    //??

    RemovePiece( p, c, from, i );
    if ( mv & CAPTURE )
    {
      tag->cap_sq = ( mv & EN_PASSANT ) ? ( to - dP[c] ) : to;
      tag->cap_p = g.pos[tag->cap_sq];
      tag->cap_p_i = g.index[tag->cap_sq];

      RemovePiece( tag->cap_p, c ^ 1, tag->cap_sq, tag->cap_p_i );


      if ( tag->cap_p == ROOK )
      {
        if ( ( c ^ 1 ) == WHITE )
        {
          if ( tag->cap_sq == A1 ) g.castl_enable &= ~WHITE_LEFT_CASTL_ENABLE;
          else if ( tag->cap_sq == H1 ) g.castl_enable &= ~WHITE_RIGHT_CASTL_ENABLE;
        }
        else
        {
          if ( tag->cap_sq == A8 ) g.castl_enable &= ~BLACK_LEFT_CASTL_ENABLE;
          else if ( tag->cap_sq == H8 ) g.castl_enable &= ~BLACK_RIGHT_CASTL_ENABLE;
        }
      }
    }

    if ( mv & ( LEFT_CASTL | RIGHT_CASTL ) )
    {
      int r_from, r_to, r_i;

      g.casling[c]++;

      if ( mv & RIGHT_CASTL )
      {
        r_from = from + 3;
        r_to = from + 1;
      }
      else
      {
        r_from = from - 4;
        r_to = from - 1;
      }
      r_i = g.index[r_from];

      RemovePiece( ROOK, c, r_from, r_i );
      InsertPiece( ROOK, c, r_to, r_i );
      //     g.moved[r_i]++;
    }

    InsertPiece( new_p, c, to, i );


    if ( p == KING )
    {
      g.castl_enable &= ~( 3 << ( c * 2 ) );

    }
    else if ( p == ROOK )
    {
      if ( c == WHITE )
      {
        if ( from == A1 ) g.castl_enable &= ~WHITE_LEFT_CASTL_ENABLE;
        else if ( from == H1 ) g.castl_enable &= ~WHITE_RIGHT_CASTL_ENABLE;
      }
      else
      {
        if ( from == A8 ) g.castl_enable &= ~BLACK_LEFT_CASTL_ENABLE;
        else if ( from == H8 ) g.castl_enable &= ~BLACK_RIGHT_CASTL_ENABLE;
      }
    }
    //   g.moved[i]++;


  }
  else{
    g.key ^= hashRnd[g.side] [PAWN] [0];
  }
  //0x001
  mtl_path[g.side][g.game_cnt+ply] = MTL(g.side);
  mtl_path[g.xside][g.game_cnt+ply] = MTL(g.xside);

  g.game_list[cnt] = mv;
  g.key_list[cnt] = g.key;
  g.side ^= 1; g.xside ^= 1;
  ply++;
  
}

void UnMakeMove( Move mv )
{
  int cnt;

  cnt = g.game_cnt + ply - 1;

  g.game_list[cnt] = 0;
  g.key_list[cnt] = 0;
  g.side ^= 1; g.xside ^= 1;
  ply--;

  if ( mv )
  {
    int from = FROM( mv ), to = TO( mv ), p = PIECE( mv ), new_p = p, i, c;
    Tag_Move * tag;

    if ( mv & PROMOTE ) p = PAWN;
    i = g.index[to];
    tag = & g.tag_list[cnt];
    c = g.side;


    g.castl_enable = tag->castl_enable;


    RemovePiece( new_p, c, to, i );
    if ( mv & CAPTURE )
    {

      InsertPiece( tag->cap_p, c ^ 1, tag->cap_sq, tag->cap_p_i );

    }

    if ( mv & ( LEFT_CASTL | RIGHT_CASTL ) )
    {
      int r_from, r_to, r_i;

      g.casling[c]--;

      if ( mv & RIGHT_CASTL )
      {
        r_from = from + 3;
        r_to = from + 1;
      }
      else
      {
        r_from = from - 4;
        r_to = from - 1;
      }
      r_i = g.index[r_to];

      RemovePiece( ROOK, c, r_to, r_i );
      InsertPiece( ROOK, c, r_from, r_i );
      //     g.moved[r_i]--;
    }

    InsertPiece( p, c, from, i );

    //   g.moved[i]--;

    tag->cap_sq = tag->cap_p = tag->cap_p_i = tag->castl_enable = 0;
  }
  else
    g.key ^= hashRnd[g.side] [PAWN] [0];
}


int atk[16 * 16], dir_atk[16 * 16];
const int center = ( 7 * 16 + 7 );

void InitAtk( void )
{
  int p, j, u, cnt;

  for ( p = KNIGHT; p <= KING; p++ )
    for ( j = d_start[p]; j <= d_stop[p]; j++ )
      for ( u = center + dir[j], cnt = 1; cnt < 8; cnt++, u += dir[j] )
      {
        atk[u] |= ( 1 << p );
        if ( p == BISHOP && cnt == 1 )
          atk[u] |= ( 1 << PAWN );
        if ( !SWEEP( p ) ) break;
        dir_atk[u] = dir[j];
      }
}

int Attack( int p, int c, int from, int to )
{
  int to_a = ( MAP( from ) - MAP( to ) ) + center, u, n1, d;

  if ( ( atk[to_a] & ( 1 << p ) ) == 0 ) return 0;

  if ( !SWEEP( p ) )
  {
    if ( p == PAWN )
    {
      if ( c == WHITE )
      {
        if ( ROW( from ) > ROW( to ) ) return 1;
      }
      else
      {
        if ( ROW( from ) < ROW( to ) ) return 1;
      }
      return 0;
    }
    return 1;
  }

  d = dir_atk[to_a];

  for ( n1 = MAP( to ) + d; ( n1 & 0x88 ) == 0; n1 += d )
  {
    u = UNMAP( n1 );
    if ( u == from ) return 1;
    if ( g.color[u] != NEUTRAL ) return 0;
  }

  return 0;
}

int SqAttack( int sq, int c )
{
  int i, from;

  for ( i = g.start[c]; i <= g.stop[c]; i++ )
    if ( ( from = g.list[i] ) != -1 && Attack( g.pos[from], c, from, sq ) )
      return 1;
  return 0;
}

int Check( void )
{

  return SqAttack( g.kingSq[g.side], g.xside );
}

int LegalMoveList( void )
{
  Move * list, * legal, * stop;
  int LegalCastl( Move mv );

  if ( Generate() == 0 ) return 0;

  list = legal = & tree[treeCnt[ply]];
  stop = & tree[treeCnt[ply + 1]];

  while ( list < stop )
  {
    Move mv = * list;
    int check = 0;

    if ( ( mv & ( LEFT_CASTL | RIGHT_CASTL ) ) && !LegalCastl( mv ) )
      check = 1;
    else
    {
      MakeMove( mv );
      check = SqAttack( g.kingSq[g.xside], g.side );
      UnMakeMove( mv );
    }

    if ( check == 0 )
    {
      * list = * legal;
      * legal = mv;
      legal++;
    }
    list++;
  }

  treeCnt[ply + 1] = legal - tree;

  return 1;//treeCnt[ply + 1] > treeCnt[ply];

}

int LegalCastl( Move mv )
{
  int c0 = g.side, c1 = c0 ^ 1;

  if ( mv & RIGHT_CASTL )
    if ( RIGHT_CASTL_ENABLE( c0 ) )
    {
      int sq = g.kingSq[c0];
      if ( ( sq == ( c0 == WHITE ? E1 : E8 ) ) && g.color[sq + 1] == NEUTRAL && g.color[sq + 2] == NEUTRAL
           && g.color[sq + 3] == c0 && g.pos[sq + 3] == ROOK && !SqAttack( sq, c1 ) && !SqAttack( sq + 1, c1 )
           && !SqAttack( sq + 2, c1 ) )
             return 1;
    }


  if ( mv & LEFT_CASTL )
    if ( LEFT_CASTL_ENABLE( c0 ) )
    {
      int sq = g.kingSq[c0];
      if ( ( sq == ( c0 == WHITE ? E1 : E8 ) ) && g.color[sq - 1] == NEUTRAL && g.color[sq - 2] == NEUTRAL
           && g.color[sq - 3] == NEUTRAL && g.color[sq - 4] == c0 && g.pos[sq - 4] == ROOK && !SqAttack( sq, c1 )
           && !SqAttack( sq - 1, c1 ) && !SqAttack( sq - 2, c1 ) )
             return 1;
    }
  return 0;
}


