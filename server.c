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

#include "joshua.h"
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int server_socket;
uint16_t port;
int pipes[FD_SETSIZE][2];
int make_server_socket(uint16_t port)
{
  int sock;
  struct sockaddr_in name;
  sock=socket(AF_INET, SOCK_STREAM, 0);
  if(sock<0)
    {
      printf("error creating socket.\n");
      return -1;
    }
  name.sin_family=AF_INET;
  name.sin_port=htons(port);
  name.sin_addr.s_addr=htonl(INADDR_ANY);
  int ret=bind(sock, (struct sockaddr*) &name, sizeof(name));
  if(ret<0)
    {
      printf("error binding to port %d\n", port);
      return -1;
    }
  return sock;
}
int process_data(int fd)
{
  char buf[1024];
  memset(buf, 0, sizeof(buf));
  int ret=read(fd, buf, sizeof(buf));
  char ctrl_c[]={0xff, 0xf4, 0xff, 0xfd, 0x06, 0x00};
  if(strcmp(ctrl_c, buf)==0)
    {
      printf("Got CTRL-C from client.\n");
      return -1;
    }
  if(ret<0) /* error */
    {
      printf("Error in read()\n");
      return -1;
    }
  if(ret==0)
    {
      printf("EOF from client\n");
      return -1;
    }
  else
    {
      printf("Client sends: %s\n", buf);
      write(pipes[fd][1], buf, strlen(buf));
      return 0;
    }
}
void serv_cleanup()
{
  printf("preparing to exit...\n");
  fflush(stdout);
  shutdown(server_socket, SHUT_RDWR);
}
int main(int argc, char* argv[])
{
  if(argc!=2)
    {
      printf("Usage: %s PORT\n", argv[0]);
      return 2;
    }
  port=atoi(argv[1]);
  printf("Initializing server on port %u...\n", port);
  signal(SIGINT, &serv_cleanup);
  int sock=make_server_socket(port);
  server_socket=sock;
  fd_set active_fd_set, read_fd_set;
  struct sockaddr_in client;
  if(listen(sock, 1)<0)
    {
      printf("Error opening socket.\n");
      return 1;
    }
  FD_ZERO(&active_fd_set);
  FD_SET(sock, &active_fd_set);
  printf("Server ready, listening on port %d.\n", port);
  while(1)
    {
      read_fd_set=active_fd_set;
      int ret=select(FD_SETSIZE, &read_fd_set, 0,0,0);
      if(ret<0)
        {
          printf("Error in select().\n");
          return 1;
        }
      for(int i=0;i<FD_SETSIZE;++i)
        {
          if(FD_ISSET(i, &read_fd_set))
            {
              if(i==sock)
                {
                  /* new connection */
                  int new, size;
                  size=sizeof(client);
                  new=accept(sock, (struct sockaddr*) &client, &size);
                  if(new<0)
                    {
                      printf("Error accepting new connection.\n");
                      return 1;
                    }
                  printf("New connection.\n");
                  FD_SET(new, &active_fd_set);
                  int ret=pipe(pipes[new]);
                  if(ret<0)
                    {
                      printf("Pipe error.\n");
                    }
                  pid_t pid=fork();
                  if(pid==0) /* child */
                    {
                      be_joshua(new);
                      printf("Client exits\n");
                      shutdown(new, SHUT_RDWR);
                      FD_CLR(new, &active_fd_set);
                      exit(0);
                    }
                }
              else
                {
                  /* data from existing connection */
                  if(process_data(i)<0)
                    {
                      shutdown(i, SHUT_RDWR);
                      FD_CLR(i, &active_fd_set);
                    }
                }
            }
        }
    }
}
