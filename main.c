/*
 * main.c
 * Copyright (C) 2022 - Evgeniy
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


//#include <condefs.h>
#include <string.h>


#ifdef __BORLANDC__
  #pragma argsused
#endif
//#pragma comment(linker, "/STACK:2000000")
//#pragma comment(linker, "/HEAP:2000000")


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
