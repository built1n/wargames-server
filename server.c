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
#include "util.h"
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
#define DEFAULT_PORT 23
int server_socket;
uint16_t port;
int pipes[FD_SETSIZE][2];
struct connection_data_t connection_data[FD_SETSIZE];
int make_server_socket(uint16_t port)
{
  int sock;
  struct sockaddr_in name;
  sock=socket(AF_INET, SOCK_STREAM, 0);
  if(sock<0)
    {
      printf("error opening socket.\n");
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
  unsigned char buf[1024];
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
      if(strlen(buf)>0)
        {
          if(buf[0]==0xff)
            {
              printf("String is command.\n");
              printf("Hex dump: ");
              for(int i=0;buf[i];++i)
                {
                  printf("%d ", buf[i]);
                }
              if(strlen(buf)<2)
                {
                  printf("Bad command (length<3).\n");
                  return 0; /* not a fatal error */
                }
              unsigned char cmd=buf[1];
#define WILL 251
#define WONT 252
#define DO 253
#define DONT 254
#define SB 250
#define SE 240
#define AYT 246
#define NAWS 31
              unsigned char opt=(strlen(buf)==2)?0:buf[2];
              unsigned char nop[2]={0xff, 241};
              switch(cmd)
                {
                case AYT:
                  /* respond with IAC NOP */
                  write(fd, nop, 2);
                  break;
                case WILL:
                case DO:
                  if(cmd==NAWS)
                    {
                      printf("Client offers NAWS\n");
                    }
                  else if(cmd==1)
                    {
                      printf("Client offers echo.\n");
                    }
                  break;
                case WONT:
                case DONT:
                  if(cmd==NAWS)
                    {
                      printf("Client denies NAWS.\n");
                    }
                  else if(cmd==1)
                    {
                      printf("Client denies echo.\n");
                    }
                  break;
                default:
                  printf("Unknown command.\n");
                  break;
                }
              if(cmd==SB)
                {
                  if(opt==NAWS)
                    {
                      if(strlen(buf)>=9)
                        {
                          uint16_t width, height;
                          uint8_t width_hi, width_lo, height_hi, height_lo;
                          width_hi=buf[3];
                          width_lo=buf[4];
                          height_hi=buf[5];
                          height_lo=buf[6];
                          width=(width_hi<<8)|width_lo;
                          height=(height_hi<<8)|height_lo;
                          connection_data[fd].know_termsize=(width==0 || height==0)?0:1; /* if any dimension is zero, unknown */
                          connection_data[fd].term_height=height;
                          connection_data[fd].term_width=width;
                          printf("Client window is %dx%d\n", width, height);
                        }
                    }
                }
            }
          else
            {
              write(pipes[fd][1], buf, strlen(buf));
            }
        }
      return 0;
    }
}
void serv_cleanup()
{
  printf("preparing to exit...\n");
  fflush(stdout);
  shutdown(server_socket, SHUT_RDWR);
}
void setup_new_connection(int fd)
{
  unsigned char will_naws[]={0xff, 251, 31};
  write(fd, will_naws, sizeof(will_naws));
  memset(&connection_data[fd], 0, sizeof(struct connection_data_t));
}
int main(int argc, char* argv[])
{
  if(argc!=2)
    {
      printf("Listening on default port: %d\n", DEFAULT_PORT);
      port=DEFAULT_PORT;
    }
  else
    {
      int port2=atoi(argv[1]);
      if(port2<0 || port2>65535)
        {
          printf("Port out of range.\n");
          return 2;
        }
      port=atoi(argv[1]);
    }
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
                      /* set up the connection */
                      setup_new_connection(new);
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
