// Network programming header
// Date: 2017.09.11

#ifndef NP_HEADER_H_
#define NP_HEADER_H_

#include <sys/types.h>  // basic system data types
#include <sys/socket.h> // basic socket definitions 
#include <sys/time.h>   // timeval{} for select() 
#include <time.h>       // timespec{} for pselect() 
#include <netinet/in.h> // sockaddr_in{} and other Internet defns 
#include <arpa/inet.h>  // inet(3) functions 
#include <fcntl.h>      // for nonblocking 
#include <netdb.h>      //
#include <signal.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/wait.h>

#define HAVE_SYS_EPOLL_H 
#define HAVE_PTHREAD_H

#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>  // for convenience 
#endif

#ifdef HAVE_POLL_H
# include <poll.h>        // for convenience 
#endif

#ifdef HAVE_SYS_EPOLL_H
# include <sys/epoll.h>
#endif

#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif

/* Libevent. */
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#ifndef HAVE_BZERO
#define bzero(ptr, n) memset(ptr, 0, n)
#endif

#define LISTENQ 1024      // 2nd argument to listen()
#define MAXLINE 4096      // max text line length 
#define MAXSOCKADDR 128   // max socket address structure size 
#define BUFFSIZE 8192     // buffer size for reads and writes 

#define SERV_IP "127.0.0.1"   // server ip

#define SERV_PORT 9988        // server port
#define SERV_PORT_STR "9988"

#define FD_LIMIT 65535
#define EPOLL_SIZE 5000
#define MAX_EPOLL_TIMEOUT_MSEC (35*60*1000)

// wrapper functions
int Socket(int family, int type, int protocol) {
  int n;

  if ((n = socket(family, type, protocol)) < 0)
    perror("socket error");
  return n;
}

void Bind(int fd, const struct sockaddr *sa, socklen_t salen) {
  if (bind(fd, sa, salen) < 0)
    perror("bind error");
}

void Listen(int fd, int backlog) {
  char *ptr;

  if ((ptr = getenv("LISTENQ")) != NULL)
    backlog = atoi(ptr);

  if (listen(fd, backlog) < 0)
    perror("listen error");
}

void Connect(int fd, const struct sockaddr *sa, socklen_t salen) {
  if (connect(fd, sa, salen) < 0)
    perror("connect error");
}

int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr) {
  int n;

  if ((n = accept(fd, sa, salenptr)) < 0) {
      perror("accept error");
  }

  return (n);
}

void Send(int fd, const void *ptr, size_t nbytes, int flags) {
  if (send(fd, ptr, nbytes, flags) != (ssize_t)nbytes)
    perror("send error");
}

ssize_t Recv(int fd, void *ptr, size_t nbytes, int flags) {
  ssize_t n;

  if ((n = recv(fd, ptr, nbytes, flags)) < 0)
    perror("recv error");
  return(n);
}


void Close(int fd) {
  if (close(fd) == -1)
    perror("close error");
}

#ifdef HAVE_SYS_SELECT_H
int Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
       struct timeval *timeout) {
  int n;

  if ((n = select(nfds, readfds, writefds, exceptfds, timeout)) < 0)
    perror("select error");
  return (n);    // can return 0 on timeout 
}
#endif

#ifdef HAVE_POLL
int Poll(struct pollfd *fdarray, unsigned long nfds, int timeout) {
  int n;

  if ((n = poll(fdarray, nfds, timeout)) < 0)
    perror("poll error");

  return (n);
}
#endif

#ifdef  HAVE_PTHREAD_H
pid_t Fork(void)
{
  pid_t pid;

  if ( (pid = fork()) == -1)
    perror("fork error");
  return(pid);
}
#endif

#endif // NP_HEADER_H_
