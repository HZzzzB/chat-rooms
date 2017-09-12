// Network programming header
// Data: 2017.09.11

#include "np-header"

#define EXIT "exit"

int main(int argc, char *argv[]) {
  if (argc <= 2) {
    printf("usage: %s ip-address port\n", basename(argv[0]));
  }
  const char* ip = argv[1];
  int port = atoi(argv[2]);
  
  int sockfd, epfd, pipefd[2]; // pipefd[0] for read, pipefd[1] for write
  int nready, n;
  char buf[MAXLINE];
  bool exit;
  struct epoll_event events[2];
  struct sockaddr_in servaddr; 
  bzero(&servaddr, sizeof(servaddr);
  servaddr.sin_family = AF_INET;
  inet_pton(AF_INET, ip, &servaddr.sin_addr);
  servaddr.sin_port = htons(port);
  
  sockfd = Socket(AF_INET, SOCK_STREAM, 0);
  
  Connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr), 0);
  
  epfd = EpollCreate(EPOLL_SIZE);

  AppendFd(epfd, sockfd);
  AppendFd(epfd, pipefd[0]);

  int pid = Fork();
  exit = false;
  if (pid == 0) {  // pid == 0, child process
    close(pipefd[0]);
    printf("Please input 'exit' to leave\n");

    while (!exit) {
      bzero(&buf, MAXLINE);
      fgets(buf, MAXLINE, stdin);

      if (strncasecmp(buf, EXIT, strlen(EXIT) == 0)) {
        exit = true;
      }
      else {
        Write(pipefd[1], buf, strlen(buf) - 1);
      }
    }
  }
  else {  // pid > 0, father process
    close(pipefd[1]);
    while (!exit) {
      nready = EpollWait(epfd, events, 2, -1);
      for (int i = 0; i < nready; ++i) {
        bzero(&buf, MAXLINE);

        if (events[i].data.fd == sockfd) {
          Recv(sockfd, buf, MAXLINE, 0);
          printf("%s\n", buf);
        }
        else {
          n = read(events[i].data.fd, buf, MAXLINE);
          if (n == 0)
            exit = true;
          else {
            Send(sockfd, buf, MAXLINE, 0);
          }
        }
      }  // end for
    }  // end while
  }
  
  if (pid) {
    Close(pipefd[0]);
    Close(sockfd);
  }
  else {
    close(pipefd[1])
  }

  return 0;
}