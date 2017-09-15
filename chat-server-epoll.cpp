// Chat room server using epoll
// Date: 2018.09.11

#include "np-header.h"  // Networt programming header file

#include <limits.h>     // for OPEN_MAX

#include <list>

#define WELCOME "Welcome to chat room! Here you can talk freely!"
#define NOTICE "Only youself in the chat room!"

#define FORMAT "user %d >> %s"

int main(int argc, char* argv[]) {
  int listenfd, connfd, sockfd, epfd;
  int nready;              // number of ready events
  int ret;                 // a temperary return value 
  ssize_t n;               // for recv() return type
  char buf[BUFFSIZE];       // read and write buffer
  socklen_t clilen;  
  struct epoll_event events[EPOLL_SIZE];  // create events entry
  struct sockaddr_in cliaddr, servaddr;

  std::list<int> evconn;

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(SERV_IP);
  servaddr.sin_port = htons(SERV_PORT);

  listenfd = Socket(AF_INET, SOCK_STREAM, 0);

  Bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

  Listen(listenfd, LISTENQ);
  printf("chat server start\n");  
  if ((epfd = epoll_create(EPOLL_SIZE)) < 0) {  // create epoll events
    perror("error at epoll_create");
    Close(listenfd);
    exit(1);
  }  

  addfd(epfd, listenfd);  // append listenfd into events

  while (true) {
    if ((nready = epoll_wait(epfd, events, EPOLL_SIZE, -1)) < 0) {
      perror("error at epoll_wait");
      break;
    }

    for (int i = 0; i < nready; ++i) {  // handle ready events
      sockfd = events[i].data.fd;

      if (sockfd == listenfd) {  // has new client connection
        bzero(&cliaddr, sizeof(cliaddr));
        clilen = sizeof(cliaddr);

        if ((connfd = accept(listenfd, (struct sockaddr*) &cliaddr, 
             &clilen)) < 0) {
          perror("error at accept");
          continue;
        }

        printf("Connection from: %s : %d, client fd: %d\n",
                inet_ntoa(cliaddr.sin_addr),
                ntohs(cliaddr.sin_port),
                connfd);

        addfd(epfd, connfd); // append new connection fd in to events

        evconn.push_back(connfd);
        printf("there are %d users in the chat room.\n", (int) evconn.size());

        if ((ret = send(connfd, WELCOME, BUFFSIZE, 0)) < 0) {
          perror("error at send");
		  Close(listenfd);
		  Close(epfd);
          exit(1);
        }
      }
      else {  
        bzero(&buf, BUFFSIZE);
        if ((n = recv(sockfd, buf, BUFFSIZE, 0)) < 0) {
          perror("error at recv");
        }
        printf("get data from user %d\n", sockfd);
        
        if (n == 0) {  // connection closed by client
          printf("user %d left the chat room\n", sockfd);
          Close(sockfd);
          evconn.remove(sockfd);
		  printf("there are %d users in the chat roon", (int)evconn.size());
        }
        else {  // Broadcast the message to all clients
          if (evconn.size() == 1) {
            if ((ret = send(sockfd, NOTICE, strlen(NOTICE), 0)) < 0) {
              perror("error at broadcast send with one client");
            }
            continue;
          }
          char message[BUFFSIZE];
		  bzero(&message, BUFFSIZE);
		  sprintf(message, FORMAT, sockfd, buf);
		  std::list<int> ::iterator it;
          for (it = evconn.begin(); it != evconn.end(); ++it) {
            if (*it != sockfd) {
              if ((ret = send((*it), message, BUFFSIZE, 0)) < 0) {
                perror("error at broadcast send");
                Close(epfd);
				Close(listenfd);
				exit(1);
              }
            }
          }
        }
        
      }

    }  // end of for
  }    // end of while

  Close(listenfd);
  Close(epfd);

  return 0;
}

