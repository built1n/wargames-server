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
#include "telnet.h"
#include "util.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#define DEFAULT_PORT 23
#define LOG_LOCATION "/var/log/wopr"
int server_socket;
uint16_t port;
int pipes[FD_SETSIZE][2];
struct connection_data_t connection_data[FD_SETSIZE];
int debugf(const char* fmt, ...)
{
  va_list l;
  va_start(l, fmt);
  int ret=vprintf(fmt, l);
  va_end(l);
  return ret;
}
int make_server_socket(uint16_t port)
{
  int sock;
  struct sockaddr_in name;
  sock=socket(AF_INET, SOCK_STREAM, 0);
  if(sock<0)
    {
      debugf("error opening socket.\n");
      return -1;
    }
  name.sin_family=AF_INET;
  name.sin_port=htons(port);
  name.sin_addr.s_addr=htonl(INADDR_ANY);
  int ret=bind(sock, (struct sockaddr*) &name, sizeof(name));
  if(ret<0)
    {
      debugf("error binding to port %d\n", port);
      return -1;
    }
  return sock;
}
char pending_buffer[1024];
void handle_command(unsigned char* buf, int buflen, int connection)
{
  unsigned char cmd, opt;
  if(buflen<2)
    {
      debugf("Invalid command.\n");
      return;
    }
  cmd=buf[1];
  /* handle two-byte commands */
  switch(cmd)
    {
    case AYT:
      {
        unsigned char iac_nop[]={IAC, NOP};
        write(connection, iac_nop, sizeof(iac_nop));
        return;
      }
    }
  if(buflen<3)
    {
      debugf("Invalid command.\n");
      return;
    }
  opt=buf[2];
  switch(cmd)
    {
    case SB:
      {
        switch(opt)
          {
          case NAWS:
            {
              /* format of NAWS data: IAC SB NAWS W W H H IAC SE */
              uint16_t height, width;
              uint8_t height_hi, height_lo, width_hi, width_lo;
              if(buflen<9)
                {
                  debugf("IAC SB NAWS command too short!\n");
                  return;
                }
              width_hi=buf[3];
              width_lo=buf[4];
              height_hi=buf[5];
              height_lo=buf[6];
              height=(height_hi<<8)|height_lo;
              width=(width_hi<<8)|width_lo;
              connection_data[connection].know_termsize=(height==0 || width==0)?0:1;
              connection_data[connection].term_height=height;
              connection_data[connection].term_width=width;
              return;
            }
          }
        break;
      }
    }
  /* unimplemented command, just deny it */
  unsigned char deny_cmd[3]={IAC, DONT, opt};
  if(opt==SGA)
    {
      deny_cmd[1]=DO;
    }
  write(connection, deny_cmd, sizeof(deny_cmd));
  fsync(connection);
  return;
}
int process_data(int fd)
{
  unsigned char buf[1024];
  memset(buf, 0, sizeof(buf));
  int ret=read(fd, buf, sizeof(buf));
  debugf("Byte dump of data: ");
  for(int i=0;buf[i];++i)
    {
      debugf("%d ", buf[i]);
    }
  debugf("\n");
  char ctrl_c[]={0xff, 0xf4, 0xff, 0xfd, 0x06, 0x00};
  if(strcmp(ctrl_c, buf)==0)
    {
      debugf("Got CTRL-C from client.\n");
      return -1;
    }
  if(ret<0) /* error */
    {
      debugf("Error in read()\n");
      return -1;
    }
  if(ret==0)
    {
      debugf("EOF from client\n");
      return -1;
    }
  else
    {
      debugf("Client sends: %s\n", buf);
      int buflen=strlen(buf);
      if(buflen>0) /* no need to write nothing to the input stream :D */
        {
          if(buf[0]==0xff)
            {
              handle_command(buf, buflen, fd);
            }
          else if(strlen(buf)>0)
            {
              write(pipes[fd][1],buf,strlen(buf));
            }
      }
      return 0;
    }
  return 0;
}
void serv_cleanup()
{
  debugf("\nPreparing to exit...\n");
  fflush(stdout);
  shutdown(server_socket, SHUT_RDWR);
}
void setup_new_connection(int fd)
{
  unsigned char do_naws[]={IAC, DO, NAWS};
  write(fd, do_naws, sizeof(do_naws));
  unsigned char dont_echo[]={IAC, DONT, ECHO};
  write(fd, dont_echo, sizeof(dont_echo));
  memset(&connection_data[fd], 0, sizeof(struct connection_data_t));
  debugf("New connection set up.\n");
}
int main(int argc, char* argv[])
{
  if(argc!=2)
    {
      debugf("Listening on default port: %d\n", DEFAULT_PORT);
      port=DEFAULT_PORT;
    }
  else
    {
      int port2=atoi(argv[1]);
      if(port2<0 || port2>65535)
        {
          debugf("Port out of range.\n");
          return 2;
        }
      port=atoi(argv[1]);
    }
  debugf("Initializing server on port %u...\n", port);
  signal(SIGINT, &serv_cleanup);
  int sock=make_server_socket(port);
  server_socket=sock;
  fd_set active_fd_set, read_fd_set;
  struct sockaddr_in client;
  if(listen(sock, 1)<0)
    {
      debugf("Error opening socket.\n");
      return 1;
    }
  FD_ZERO(&active_fd_set);
  FD_SET(sock, &active_fd_set);
  debugf("Server ready, listening on port %d.\n", port);
  debugf("Maximum clients: %d\n", FD_SETSIZE);
  while(1)
    {
      read_fd_set=active_fd_set;
      int ret=select(FD_SETSIZE, &read_fd_set, 0,0,0);
      if(ret<0)
        {
          debugf("Error in select().\n");
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
                      debugf("Error accepting new connection.\n");
                      return 1;
                    }
                  debugf("New connection, number %d.\n", new);
                  FD_SET(new, &active_fd_set);
                  int ret=pipe(pipes[new]);
                  if(ret<0)
                    {
                      debugf("Pipe error.\n");
                    }
                  pid_t pid=fork();
                  if(pid==0) /* child */
                    {
                      /* set up the connection */
                      setup_new_connection(new);
                      be_joshua(new);
                      debugf("Client exits\n");
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
