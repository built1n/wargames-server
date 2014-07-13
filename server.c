#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "joshua.h"
#define PORT 1029
int server_socket;
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
  if(ret<0) /* error */
    {
      printf("error in read()\n");
      return -1;
    }
  if(ret==0)
    {
      printf("EOF from client\n");
      return -1;
    }
  else
    {
      write(pipes[fd][1], buf, strlen(buf));
    }
}
void serv_cleanup()
{
  printf("preparing to exit...\n");
  fflush(stdout);
  close(server_socket);
}
int main(int argc, char* argv[])
{
  printf("starting server...\n");
  signal(SIGINT, &serv_cleanup);
  int sock=make_server_socket(PORT);
  server_socket=sock;
  fd_set active_fd_set, read_fd_set;
  struct sockaddr_in client;
  if(listen(sock, 1)<0)
    {
      printf("error listening.\n");
      return 1;
    }
  FD_ZERO(&active_fd_set);
  FD_SET(sock, &active_fd_set);
  printf("listening on port %d\n", PORT);
  while(1)
    {
      read_fd_set=active_fd_set;
      int ret=select(FD_SETSIZE, &read_fd_set, 0,0,0);
      if(ret<0)
        {
          printf("select() returned error.\n");
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
                      printf("error accepting connection.\n");
                      return 1;
                    }
                  printf("new connection.\n");
                  FD_SET(new, &active_fd_set);
                  int ret=pipe(pipes[i]);
                  if(ret<0)
                    {
                      printf("pipe error.\n");
                    }
                  pid_t pid=fork();
                  if(pid==0) /* child */
                    {
                      printf("child.\n");
                      be_joshua(new);
                      close(new);
                      FD_CLR(new, &active_fd_set);
                      exit(0);
                    }
                }
              else
                {
                  /* data from existing connection */
                  if(process_data(i)<0)
                    {
                      close(i);
                      FD_CLR(i, &active_fd_set);
                    }
                }
            }
        }
    }
}
