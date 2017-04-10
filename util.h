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

#define SLEEP_TIME 5000

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

struct connection_data_t {
  int naws_enable : 1;
  int know_termsize : 1;
  uint16_t term_height;
  uint16_t term_width;
};

extern struct connection_data_t connection_data[FD_SETSIZE];

void allLower(char*);
void print_string(const char*);
void remove_punct(char*);
void refresh(void);
void clear(void);
int getnstr(char*, int);
void echo_off(void);
void echo_on(void);

#define ERR 1
#define OK 0

extern int out_fd;
