#include "chess.h"

#define LEARN_FILE "learnch12.dat"
#define BOOK_FILE "Kaissa.bk"


void PlayWinBoard( char * prog_dir )
{
  char s[1024];
#define LEX_IS(s)  (strcmp(s,lex)==0)
  void InitNewGame( void );
  int StrToMove( char * s, Move * ret_mv );
  int SearchMove( Root_Score * ret );
  int InitGame( char * epd );
  void ReadLibFile( char * fname );
  int SaveLearnTable( char * fname );
  int LoadLearnTable( char * fname );
  char last_game_file_name[256], book_file_name[256], learn_file_name[256];
  Move mv;
  Root_Score search_result;
  int stop = 0;

  setbuf( stdout, NULL );
  setbuf( stdin, NULL );

  InitAllVar();
  InitNewGame();

  sprintf( last_game_file_name, "%s%s", prog_dir, "game00.dat" );
  sprintf( book_file_name, "%s%s", prog_dir, BOOK_FILE );
  //sprintf( learn_file_name, "%s%s", prog_dir, LEARN_FILE );
  sprintf( learn_file_name, "%s%s", prog_dir, LEARN_FILE );

  //LearnClear();
  //ReadLibFile( book_file_name );
  //load learn table

  if( LoadLearnTable( learn_file_name ) == 0 ) {
    void LearnClear(void);

    LearnClear();
    ReadLibFile( book_file_name );
  }


  //init interface with arena (winboard)
  //printf( "feature setboard=1\n" );
  //printf( "feature done=1\n" );


  while ( fgets( s, sizeof( s ) - 1, stdin ) )
  {
    char * lex = strtok( s, " \n" ), * pen;//= "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2";
    //= "r4rk1/4Rp2/6p1/1p1p1bNp/p1qP3P/2P1Q3/P4PP1/4R1K1 b"; // - - 0 0";

    if ( lex )
    {
      if ( LEX_IS( "quit" ) ) break;
      else if ( LEX_IS( "st" ) )
      {
        extern time_t limit_time;
        char * s = strtok( 0, " \n" );
        limit_time = atoi( s ) * 100;
        if ( limit_time <= 0 ) limit_time = 5 * 100;
      }
      else if( LEX_IS("xboard") ){
          printf( "feature setboard=1\n" );
          printf( "feature done=1\n" );
          printf( "feature sigint=0\n" ); // only for Linux
          printf( "feature sigterm=0\n" );  

      }else if( LEX_IS( "time") )
      {
        extern time_t limit_time;
        char * s = strtok( 0, " \n" );
        int t_error = 0;
        if(s){
          int t = atoi( s );
          if( t >= 0 ){
            int cnt = 40 - (g.game_cnt/2) % 40;
            int new_limit = t / cnt;
            limit_time = new_limit;
            if(limit_time < 2.5 * 100) limit_time = 2.5 * 100;
            else if(limit_time > 40 * 100) limit_time = 40 * 100;

///////////////////  bug fixed aproximately ///////
            /// ??? see  JA fixed
            if(g.side == BLACK && cnt == 1 )
            {
              if( limit_time > 3*100)
              limit_time = 3*100; // shit
            }


///////////////////  end bug fixed aproximately ///////

          } else t_error = 1;
        } else t_error = 1;

        if(t_error){
          fprintf(stderr,"#error time value\n");

        }
      }
      else if ( LEX_IS( "score" ) )
      {
        int Evaluate( int alpha, int beta );
        void InitEvaluate( void );

        InitEvaluate();
        printf( "# %d\n", Evaluate( -INF, INF ) );

      }
      else if ( LEX_IS( "force" ) )
      {

        stop = 1;

      }
      else if ( LEX_IS( "undo" ) )
      {
        if ( g.game_cnt > 0 )
        {
          Move mv = g.game_list[g.game_cnt - 1];
          UnMakeMove( mv );
          g.game_list[g.game_cnt - 1] = mv;
          g.game_cnt--;
          ply = 0;
        }

      }
      else if ( LEX_IS( "redo" ) )
      {

        if ( g.game_cnt < g.game_max )
        {
          MakeMove( g.game_list[g.game_cnt] );
          g.game_cnt++;
          ply = 0;

        }

      }
      else if ( LEX_IS( "new" ) )
      {
        InitNewGame();

      }
      else if ( LEX_IS( "protover" ) )
      {
        printf( "feature done=1\n" );
      }
      else if( LEX_IS("deb") && InitGame(
       "r4rk1/4Rp2/6p1/1p1p1bNp/p1qP3P/2P1Q3/P4PP1/4R1K1 b - - 0 0"
      )){


      }

      else if ( LEX_IS( "setboard" ) &&  (pen = strtok(NULL,"\n")) != NULL
           && InitGame( pen ) )
           {
             /* void MakeEpd(char *s); char s[1024]; extern int __foo; MakeEpd(s);

             if(strcmp(s,pen)==0){ __foo++; } */
      }
      else if ( StrToMove( lex, & mv ) && InsertMoveInGame( mv ) )
      {

        if ( !stop )
          if ( SearchMove( & search_result ) && InsertMoveInGame( search_result.mv ) )
          {
            char str_move[64];
            char * MoveToStr( Move mv, char * str );
            printf( "move %s\n", MoveToStr( search_result.mv, str_move ) );
          }


      }
      else if ( LEX_IS( "go" ) && SearchMove( & search_result ) && InsertMoveInGame( search_result.mv ) )
      {
        char str_move[64];
        char * MoveToStr( Move mv, char * str );

        printf( "move %s\n", MoveToStr( search_result.mv, str_move ) );
        stop = 0;
      }
      else
      {
        fprintf( stderr, "#error: %s\n", lex );
      }



    } //if lex
  } //while


  SaveLearnTable( learn_file_name );
}

