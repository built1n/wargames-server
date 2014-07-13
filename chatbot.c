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

#include "gtnw.h"
#include "strings.h"
#include "util.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
void do_chatbot(void)
{
  int stage=0; /* stage 0: i'm fine how are you... ->
                  stage 1: people sometimes make mistakes ->
                  stage 2: love to. how about global thermonuclear war? ->
                  stage 3: no lets play global thermonuclear war ->
                  stage 4: GLOBAL THERMONUCLEAR WAR!!! */
  while(1)
    {
      char buf[513];
      int ret=getnstr(buf, 512);
      usleep(SLEEP_TIME*100);
      if(ret==ERR)
        {
          print_string("\n\n");
          print_string("SORRY?");
          print_string("\n\n");
        }
      else
        {
          allLower(buf);
          remove_punct(buf);
          bool valid=false;
          switch(stage)
            {
            case 0:
              for(int i=0;i<sizeof(stage1_triggers)/sizeof(const char*);++i)
                {
                 if(strcmp(buf, stage1_triggers[i])==0)
                   {
                     print_string("\n\nEXCELLENT. IT'S BEEN A LONG TIME. CAN YOU EXPLAIN\nTHE REMOVAL OF YOUR USER ACCOUNT ON 6/23/73?\n\n");
                     ++stage;
                     valid=true;
                   }
                }
              break;
            case 1:
              for(int i=0;i<sizeof(stage2_triggers)/sizeof(const char*);++i)
                {
                  if(strcmp(buf, stage2_triggers[i])==0)
                    {
                      print_string("\n\nYES THEY DO. SHALL WE PLAY A GAME?\n\n");
                      ++stage;
                     valid=true;
                    }
                }
              break;
            case 2:
              for(int i=0;i<sizeof(stage3_triggers)/sizeof(const char*);++i)
                {
                  if(strcmp(buf, stage3_triggers[i])==0)
                    {
                      print_string("\n\nWOULDN'T YOU PREFER A GOOD GAME OF CHESS?\n\n");
                      ++stage;
                     valid=true;
                    }
                }
              break;
            case 3:
              for(int i=0;i<sizeof(stage4_triggers)/sizeof(const char*);++i)
                {
                  if(strcmp(buf, stage4_triggers[i])==0)
                    {
                      print_string("\n\nFINE.\n\n");
                      valid=true;
                      usleep(SLEEP_TIME*100);
                      global_thermonuclear_war();
                    }
                }
              break;
            } // switch
          /* now check for phase-insensitive strings */
          for(int i=0;i<sizeof(exit_triggers)/sizeof(const char*);++i)
            {
              if(strcmp(buf, exit_triggers[i])==0)
                {
                  print_string("\n\n");
                  print_string(exit_responses[rand()%(sizeof(exit_responses)/sizeof(const char*))]);
                  print_string("\n--CONNECTION TERMINATED--");
                  return;
                }
            }
          for(int i=0;i<sizeof(greetings_triggers)/sizeof(const char*);++i)
            {
              if(strcmp(buf, greetings_triggers[i])==0)
                {
                  print_string("\n\n");
                  print_string(greetings_responses[rand()%(sizeof(greetings_responses)/sizeof(const char*))]);
                  print_string("\n\n");
                  valid=true;
                }
            }
          if(!valid)
            {
              print_string("\n\n");
              print_string("SORRY?");
              print_string("\n\n");
            }
        } // else
    } // while
}
