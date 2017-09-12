// Chat room server using epoll
// Date: 2018.09.11

#include "np-header.h"  // Networt programming header file

#define WELCOME "Welcome to chat room! Here you can talk freely!"
#define NOTICE "You are speaking to youself!"

#define USER_LIMIT 5

struct client_data{
  sockaddr_in addr;
  char *write_buf;
  char buf[MAXLINE];
};

void addfd(int epfd, int fd);  // add fd to events
int setnonblock(int fd);      // set socket to non-blocking mode

int main(int argc, char *argv[]) {
  int listenfd, connfd, sockfd, epfd;     // fds
  int nready;                             // number of ready events
  int ret;                                // a temperary return value 
  ssize_t n;                              // for recv() return type
  socklen_t clilen;  
  struct epoll_event events[USER_LIMIT];  // create events entry
  struct sockaddr_in cliaddr, servaddr;

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(SERV_IP);
  servaddr.sin_port = htons(SERV_PORT);

  listenfd = Socket(AF_INET, SOCK_STREAM, 0);

  Bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

  Listen(listenfd, LISTENQ);
  
  if ((epfd = epoll_create(USER_LIMIT)) < 0) {  // create epoll events
    perror("error at epoll_create");
    Close(listenfd);
    exit(1);
  }  

  addfd(epfd, listenfd);  // append listenfd into events

  client_data *users = new client_data[USER_LIMIT];
  int user_counter = 0;

  for (int i = 0; i < USER_LIMIT; ++i) {
    events[i].data.fd = -1;
    events[i].events = 0;
  }

  while (true) {
    if ((nready = epoll_wait(epfd, events, USER_LIMIT, -1)) < 0) {
      perror("error at epoll_wait");
      break;
    }

    for (int i = 0; i < nready; ++i) {  // handle ready events
      sockfd = events[i].data.fd;

      // has new client connection
      if (sockfd == listenfd && events[i].events & EPOLLIN) {  
        bzero(&cliaddr, sizeof(cliaddr));
        clilen = sizeof(cliaddr);

        if ((connfd = accept(listenfd, (struct sockaddr*) &cliaddr, 
             &clilen)) < 0) {
          printf("errno is: %d\n", errno);
          continue;
        }

        if (user_counter >= USER_LIMIT) {
          const char* info = "Too many users\n";
          printf("%s\n", info);
          Send(connfd, info, strlen(info), 0);
          Close(connfd);
          continue;
        }
        ++user_counter;
        users[connfd].addr = cliaddr;
        // Set nonblocking IO
        fcntl(connfd, F_SETFL, fcntl(connfd, F_GETFD, 0) | O_NONBLOCK); 
        events[user_counter].data.fd = connfd;
        events[user_counter].events = EPOLLOUT | EPOLLHUP | EPOLLERR | EPOLLET;
        
        if ((epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, 
             &events[user_counter])) < 0) {
          perror("error at epoll_ctl");
        }

        printf("Connection from: %s : %d, client fd: %d\n",
                inet_ntoa(cliaddr.sin_addr),
                ntohs(cliaddr.sin_port),
                connfd);

        printf("There are %d users in the chat room.\n", (int) evconn.size());

        if ((ret = send(connfd, WELCOME, MAXLINE, 0)) < 0) {
          perror("error at send");
        }
      }
      else if (events[i].events & EPOLLHUP) { // user close connection
        users[events[i].data.fd] = users[events[user_counter].data.fd];
        printf("Client %d left\n", events[i].data.fd);
        Close(events[i].data.fd);
        events[i] = events[user_counter];
        --i;
        --user_counter;
      }
      else if (events[i].events & EPOLLOUT) { // send data
        connfd = events[i].data.fd;
        memset(users[connfd].buf, '\0', MAXLINE);
        ret = recv(connfd, users[connfd].buf, MAXLINE - 1, 0);
        printf("%s\n", users[connfd].buf);

        if (ret < 0) {
          if (errno != EAGAIN) {
            Close(connfd);
            users[events[i].data.fd] = users[events[user_counter].data.fd];
            events[i] = events[user_counter];
            --i;
            --user_counter;
          }
        }
        else if (ret == 0) {

        }
        else {
          for (int j = 0; j < user_counter; ++j) {
            if (events[j].data.fd == connfd) {
              continue;
            }

            events[j].events |= ~EPOLLIN;
            events[j].events | = EPOLLOUT;
            users[events[j].data.fd].write_buf = users[connfd].buf;
            epoll_ctl(epfd, EPOLL_CTL_MOD, connfd, &events[j]);
          }
        }
      }
      else if (events[i].events & EPOLLOUT) {
        connfd = events[i].data.fd;
        if (!users[connfd].write_buf) {
          continue;
        }
        ret = Send(connfd, users[connfd].write_buf, 
                   strlen(users[connfd].write_buf), 0);
        users[connfd].write_buf = NULL;
        events[i].events |= ~EPOLLOUT;
        events[i].events | = EPOLLIN;
        epoll_ctl(epfd, EPOLL_CTL_MOD, connfd, &events[i]);
      }
      else {
        perror("error at else");
      }

    }  // end of for
  }    // end of while

  Close(listenfd);
  Close(epfd);
  delete [] users;
  return 0;
}

int setnonblock(int fd) {
  int n;
  n = fcntl(fd, F_GETFL);
  if (n < 0) {
    perror("error at fcntl");
    return n;
  }
  n |= O_NONBLOCK；
  if ((n = fcntl(fd, F_SETFL)) < 0) {
    perror(error at fcntl);
    return n;
  }
  return n;
}

void addfd(int epfd, int fd) {
  struct epoll_event epev;
  epev.data.fd = fd;
  epev.events = EPOLLIN | EPOLLET;  // ET mode
  
  if ((epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &epev)) < 0) {
    perror("error at epoll_ctl");
  }

  fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK); // Set nonblocking IO
}


