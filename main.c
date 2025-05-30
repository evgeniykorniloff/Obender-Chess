//#include <condefs.h>
#include <string.h>
#include <stdio.h>

#ifdef __BORLANDC__
  #pragma argsused
#endif

extern  void PlayWinBoard(char *prog_dir);


void print_info(void)
{
 int msg[] = {
   'C'<<1, 'h'<<1, 'e'<<1, 's'<<1, 's'<<1, ' '<<1,
   'b'<<1, 'y'<<1, ' '<<1,
   'N'<<1, 'i'<<1, 'f'<<1, 'o'<<1, 'n'<<1, 't'<<1, ' '<<1, 'I'<<1,
   //'K'<<1, 'o'<<1, 'r'<<1, 'n'<<1, 'i'<<1, 'l'<<1,
   //'o'<<1, 'f'<<1, 'f'<<1, ' '<<1, 'E'<<1, 
   0 };
   int j;


   for(j = 0; msg[j]; j++)
	   printf("%c", msg[j]>>1);

   printf("\n");
   
}



int main(int argc, char *argv[])
{

  char dir[1024];


  setbuf( stdout, NULL );
  setbuf( stdin, NULL );

  print_info();

  strcpy(dir,argv[0]);

  if(strrchr(dir,'\\')) *(strrchr(dir,'\\')+1) = '\0';
  else   if(strrchr(dir,'/')) *(strrchr(dir,'/')+1) = '\0';
  else dir[0] = '\0';

  PlayWinBoard(dir);

  return 0;
}
