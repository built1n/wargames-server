/*
 *   WarGames - a WOPR emulator written in C
 *   Copyright (C) 2014 Franklin Wei
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Contact the author at contact@fwei.tk
 */

#include "strings.h"
#include "util.h"
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
int out_fd;
extern int pipes[FD_SETSIZE][2];
void allLower(char* str)
{
  for(int i=0;str[i];++i)
    {
      str[i]=tolower(str[i]);
    }
}
void print_string(const char* str) /* print string, slowly */
{
  int i=0;
  while(str[i])
    {
      write(out_fd, &str[i], 1);
      fsync(out_fd);
      ++i;
    }
}
void remove_punct(char* buf)
{
  for(int i=0;buf[i];++i)
    {
      for(int j=0;j<sizeof(punctuation_marks)/sizeof(char);++j)
        {
          if(buf[i]==punctuation_marks[j])
            {
              memmove(&buf[i], &buf[i+1], strlen(buf)-i);
            }
        }
    }
}
void clear(void)
{
}
void refresh(void)
{
  fsync(out_fd);
}
int getnstr(char* buf, int max)
{
  printf("reading...\n");
  memset(buf, 0, sizeof(buf));
  int back=0;
  char c=0;
  do {
    read(pipes[out_fd][0], &c, 1);
    if(c!='\n')
      {

        buf[back]=c;
        ++back;
      }
  }
  while(back<max && c!='\n');
  return OK;
}
