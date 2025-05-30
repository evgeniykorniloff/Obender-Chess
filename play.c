#include "chess.h"

#define LEARN_FILE "learnch12.dat"
#define BOOK_FILE "Kaissa.bk"


void show_board(void){

   const char *chp[2] = 
   {
     " PNBRQK",
     " pnbrqk" 
   };
   const int black_sq[64] = 
   {
     0, 1, 0, 1, 0, 1, 0, 1,
     1, 0, 1, 0, 1, 0, 1, 0,
     0, 1, 0, 1, 0, 1, 0, 1,
     1, 0, 1, 0, 1, 0, 1, 0,
     0, 1, 0, 1, 0, 1, 0, 1,
     1, 0, 1, 0, 1, 0, 1, 0,
     0, 1, 0, 1, 0, 1, 0, 1,
     1, 0, 1, 0, 1, 0, 1, 0
   };
   const char rank[8] = {'8', '7', '6', '5', '4', '3', '2', '1'};
   const char file[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
   
   
   int x,y,sq,p;
   
   for(y = 0; y < 8; y++)
   {
   
     printf("%c  ", rank[y]);
     
     for(x = 0; x < 8; x++)
     {
        sq = y*8 + x;
        p = g.pos[sq];
        if(p==NOPIECE)
        {
           printf("%c ", black_sq[sq] ? ' ':'-'); 
        
        }else{
           printf("%c ", chp[ g.color[sq]][p]);        
        
        }
     }
     printf("\n");
   }
   
   printf("\n   ");
   for(x = 0; x < 8; x++)
     printf("%c ", file[x]);
   printf("\n");
     
     
   printf("side:  %s\n",  g.side==WHITE?"white":"black");
}



void PlayWinBoard( char * prog_dir )
{
  char s[1024];
#define LEX_IS(s)  (strcmp(s,lex)==0)
  void InitNewGame( void );
  int prog_is_run(void);
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
  int prev_inst = prog_is_run();



 

  InitAllVar();
  InitNewGame();

  sprintf( last_game_file_name, "%s%s", prog_dir, "game00.dat" );
  sprintf( book_file_name, "%s%s", prog_dir, BOOK_FILE );
  //sprintf( learn_file_name, "%s%s", prog_dir, LEARN_FILE );
  sprintf( learn_file_name, "%s%s", prog_dir, LEARN_FILE );


  //load learn table
  if ( LoadLearnTable( learn_file_name ) == 0 ) {
    void LearnClear(void);
    
    LearnClear();
    ReadLibFile( book_file_name );

  }

  //init interface with arena (winboard)
  printf( "feature setboard=1\n" );
  printf( "feature done=1\n" );


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
      else if( LEX_IS( "time") )
	  {

        extern time_t limit_time;
        char * s = strtok( 0, " \n" );
        int t_error = 0;
        if(s){
          int t = atoi( s );
          int cnt;
          if( t >= 0 ){
            if (g.side==BLACK) {  // fix for 'timeout playing as black bug' - JA
              cnt = 40 - ((g.game_cnt-2)/2) % (40-2); 
            }else{
              cnt = 40 - (g.game_cnt/2) % 40; 
            }
            limit_time = (t/cnt) *99/100;
            
          } else t_error = 1;
        } else t_error = 1;

        if(t_error){
          fprintf(stderr,"#error time value\n");

        }



		  /*
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
		*/
      }
      else if ( LEX_IS( "pos" ) )
      {
        show_board();
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
      else if( LEX_IS("xboard") ){
          printf( "feature setboard=1\n" );
          printf( "feature done=1\n" );
          printf( "feature sigint=0\n" ); // only for Linux
          printf( "feature sigterm=0\n" );  

      }
      else if ( LEX_IS( "new" ) )
      {
        InitNewGame();
		stop = 0;

      }
      else if ( LEX_IS( "protover" ) )
      {
        printf( "feature done=1\n" );
      }
      else if ( LEX_IS( "deb" ) )
      {
         char *p = strtok(NULL," \n"); 
		 char *pos = "";
         if(p)
		  switch(atoi(p))
		  {
		     case 1:  pos = "r4rk1/4Rp2/6p1/1p1p1bNp/p1qP3P/2P1Q3/P4PP1/4R1K1 b - - 0 0";
				      break;
		     case 2:  pos = "r1bq1r1k/ppp2pp1/2nbp2p/6NQ/3P4/2PB4/P1P2PPP/R1B2RK1 b - - 0 12";
				      break;
		     case 3:  pos = "2r2b1r/p1qk1ppp/2p1pn2/1B6/Pp6/4PP2/1PQB1P1P/2R2RK1 w - - 0 16 "; 
				      break;
		     case 4:  pos = "r5k1/7p/1Rp2rp1/2Pp4/P2Qp3/4PqP1/3R1P1P/6K1 w - - 0 38";
				      break;
			 case 5: pos = "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2";
				      break;
			 case 6: pos = "rnbqkbnr/ppp1pppp/8/8/2pP4/8/PP2PPPP/RNBQKBNR w KQkq - 0 3";
				      break;
			 case 7: pos = "r1b1kb1r/ppp1n3/3q4/4pp2/4Q1p1/3B4/PPPP1PP1/R1B1NRK1 w kq - 0 15";
				 break;
			 case 8: pos = "4b3/2R3B1/5P2/pk5p/2p2r1P/P1K5/1P6/8 w - - 0 63";
				  break;
			 case 9: pos = "r3kb1r/ppp1pppp/2n2n2/q7/2BP2b1/2N5/PPP1NPPP/R1BQK2R w KQkq - 5 7 ";
				  break;
		 	 case 10: pos = "r3kb1r/ppp1pppp/2n2n2/q4b2/2BP4/2N2P2/PPP1N1PP/R1BQK2R w KQkq - 1 8 ";
				 break;
		 }    
         
		 if(InitGame(pos)==0) fprintf(stderr,"#error\n");
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

if( prev_inst==0)
   SaveLearnTable( learn_file_name );
}

