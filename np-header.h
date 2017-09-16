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
#include <assert.h>

#include <unistd.h>
#include <sys/wait.h>

#include <sys/mman.h>   // for shared memory
#include <sys/stat.h>   // 

#define HAVE_SYS_SELECT
#define HAVE_SYS_EPOLL
#define HAVE_POLL
#define HAVE_PTHREAD

#ifdef HAVE_SYS_SELECT
# include <sys/select.h>  // for convenience 
#endif

#ifdef HAVE_POLL
# include <poll.h>        // for convenience 
#endif

#ifdef HAVE_SYS_EPOLL
# include <sys/epoll.h>
#endif

#ifdef HAVE_PTHREAD
# include <pthread.h>
#endif

/* Libevent. */
#include <event2/event_struct.h>
#include <event2/bufferevent.h> 
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

#ifndef HAVE_BZERO
#define bzero(ptr, n) memset(ptr, 0, n)
#endif

#define LISTENQ 1024      // 2nd argument to listen()
#define MAXSOCKADDR 128   // max socket address structure size 
#define BUFFSIZE 4096     // buffer size for reads and writes 

#define SERV_IP "127.0.0.1"   // server ip

#define SERV_PORT 8008        // server port
#define SERV_PORT_STR "8008"

#define FD_LIMIT 65535
#define EPOLL_SIZE 8192
#define PROCESS_LIMIT 65535
#define MAX_EPOLL_TIMEOUT_MSEC (35*60*1000)

// wrapper functions
int Socket(int family, int type, int protocol) {
  int n;

  if ((n = socket(family, type, protocol)) < 0)
    perror("socket error");
  return n;
}

int Bind(int fd, const struct sockaddr *sa, socklen_t salen) {
  int n;
  if ((n = bind(fd, sa, salen)) < 0)
    perror("bind error");

  return n;
}

void Listen(int fd, int backlog) {
  char *ptr;

  if ((ptr = getenv("LISTENQ")) != NULL)
    backlog = atoi(ptr);

  if (listen(fd, backlog) < 0)
    perror("listen error");
}

int Connect(int fd, const struct sockaddr *sa, socklen_t salen) {
  int n;

  if ((n = connect(fd, sa, salen)) < 0)
    perror("connect error");
  return n;
}

int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr) {
  int n;

  if ((n = accept(fd, sa, salenptr)) < 0) {
      perror("accept error");
  }

  return n;
}

void Send(int fd, const void *ptr, size_t nbytes, int flags) {
  if (send(fd, ptr, nbytes, flags) != (ssize_t)nbytes)
    perror("send error");
}

ssize_t Recv(int fd, void *ptr, size_t nbytes, int flags) {
  ssize_t n;

  if ((n = recv(fd, ptr, nbytes, flags)) < 0)
    perror("recv error");
  return n;
}


void Close(int fd) {
  if (close(fd) == -1)
    perror("close error");
}

#ifdef HAVE_SYS_SELECT
int Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
       struct timeval *timeout) {
  int n;

  if ((n = select(nfds, readfds, writefds, exceptfds, timeout)) < 0)
    perror("select error");
  return n;    // can return 0 on timeout 
}
#endif

#ifdef HAVE_POLL
int Poll(struct pollfd *fdarray, unsigned long nfds, int timeout) {
  int n;

  if ((n = poll(fdarray, nfds, timeout)) < 0)
    perror("poll error");

  return n;
}
#endif

#ifdef HAVE_SYS_EPOLL
int setnonblock(int fd) {
  int n;
  n = fcntl(fd, F_GETFL);
  if (n < 0) {
    perror("error at fcntl");
    return n;
  }
  n |= O_NONBLOCK;
  if ((n = fcntl(fd, F_SETFL)) < 0) {
    perror("error at fcntl");
    return n;
  }
  return n;
}

void addfd(int epfd, int fd) {  // epoll ET mode
  struct epoll_event epev;
  epev.data.fd = fd;
  epev.events = EPOLLIN | EPOLLET;  // ET mode
  
  if ((epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &epev)) < 0) {
    perror("error at epoll_ctl");
  }

  setnonblock(fd);
}
#endif

#ifdef  HAVE_PTHREAD
pid_t Fork(void)
{
  pid_t pid;

  if ( (pid = fork()) == -1)
    perror("fork error");
  return pid;
}
#endif

#endif // NP_HEADER_H_
