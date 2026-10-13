#include <stdio.h>
#ifndef __MINGW32__ 
#include <termios.h>
#else
#include <conio.h> //_kbhit, _getch
#include <stdlib.h> //exit
#endif
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include "sim.h"

int kbhit(void)
{
#ifndef __MINGW32__ 
  struct timeval tv;
  fd_set rdfs;

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  FD_ZERO(&rdfs);
  FD_SET(STDIN_FILENO, &rdfs);

  select(STDIN_FILENO + 1, &rdfs, NULL, NULL, &tv);
  return FD_ISSET(STDIN_FILENO, &rdfs);
#else
  return _kbhit();
#endif  
}

int osd_get_char()
{
  int ch = -1;
  while (kbhit())
#ifndef __MINGW32__ 
    ch = getchar();
#else
    ch = _getch();
    //if (ch > 0) printf("====>%d\n", ch);
    if (ch == 27) exit(0); //ESC to exit
#endif    
  return ch;
}
