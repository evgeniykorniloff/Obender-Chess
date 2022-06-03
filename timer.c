//#include <windows.h>
#include  "chess.h"

#ifdef  _WINDOWS_
    #define TICK_COUNT  (GetTickCount()/10)
#else
    #define TICK_COUNT  (time(0)*100)
#endif



time_t start_time,curr_time,limit_time = 7.5*100;
int is_stop_search;;
U64 cnt_nodes;




void TimeReset(void){
  start_time = curr_time = TICK_COUNT;
  cnt_nodes = 0;
  is_stop_search = 0;
}

int StopSearch(void){


   if( (++cnt_nodes & 0xFFF)==0  &&  !is_stop_search){
      curr_time = TICK_COUNT;
      if(curr_time < start_time) start_time = curr_time;
      if(curr_time - start_time >= limit_time)
       is_stop_search = 1;
   }
   return is_stop_search;
}
