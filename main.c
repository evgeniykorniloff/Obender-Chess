/****
  xboard (winboard) chess engine, version alpha
  run:
     xboard -fcp obender

  evgeniy-korniloff@yandex.ru

  compile in windows:
    incomment <windows.h> file
*****/
//#include <condefs.h>
#include <string.h>


#ifdef __BORLANDC__
  #pragma argsused
#endif

extern  void PlayWinBoard(char *prog_dir);
int main(int argc, char **argv)
{

  char dir[1024];


  strcpy(dir,argv[0]);

  if(strrchr(dir,'\\')) *(strrchr(dir,'\\')+1) = '\0';
  else   if(strrchr(dir,'/')) *(strrchr(dir,'/')+1) = '\0';
  else dir[0] = '\0';

  PlayWinBoard(dir);

  return 0;
}
