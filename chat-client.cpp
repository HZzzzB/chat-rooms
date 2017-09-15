// A client app for chat room
// Data: 2017.09.09

#include "np-header.h"

#define EXIT "EXIT"

int main(int argc, char* argv[]) {
  int sockfd, epfd, pipefd[2];
  struct sockaddr_in servaddr;
  static struct epoll_event events[2]; // 
  
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(SERV_IP);
  servaddr.sin_port = htons(SERV_PORT);

  sockfd = Socket(AF_INET, SOCK_STREAM, 0);

  if (Connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) {
    exit(1);
  }
   
  if (pipe(pipefd) < 0) {
    perror("error at pipe");
  }

  if ((epfd = epoll_create(EPOLL_SIZE)) < 0) {
    perror("error at epoll_create");
    Close(sockfd);
    exit(1);
  }

  addfd(epfd, sockfd);
  addfd(epfd, pipefd[0]);

  bool clirun = true;
  char message[BUFFSIZE];
  pid_t pid = Fork();
  if (pid < 0) {
    Close(sockfd);
    Close(epfd);
    exit(1);
  }
  else if (pid == 0) {
    Close(pipefd[0]);
    printf("Please enter 'exit' to exit the chat room\n");

    while (clirun) {
      bzero(&message, BUFFSIZE);
      fgets(message, BUFFSIZE, stdin);
      
      if (strncasecmp(message, EXIT, strlen(EXIT)) == 0) {
        clirun = false;
      }
      else {
        if (write(pipefd[1], message, strlen(message) - 1) < 0) {
          perror("error at fork");
          Close(sockfd);
          Close(epfd);
          exit(1);
        }
      }
    }

  }
  else {  // pid > 0, parent process
    Close(pipefd[1]); // parent process read data from pipe

    while (clirun) {
      int nfds = epoll_wait(epfd, events, 2, -1);

      for (int i = 0; i < nfds; ++i) {
        bzero(&message, BUFFSIZE);
        // get data from server
        if (events[i].data.fd == sockfd) {
          ssize_t n = Recv(sockfd, message, BUFFSIZE, 0);
          
		  if (n == 0) {
            printf("server closed conection\n");
            Close(sockfd);
            clirun = false;
          }
          else {
            printf("%s\n", message);
          }
        }
        else {
          ssize_t n = read(events[i].data.fd, message, BUFFSIZE);

          if (n == 0) {
            clirun = false;
          }
          else {
            Send(sockfd, message, BUFFSIZE, 0);
          }
        }

      }
    }
  }

  if (pid) {
    Close(pipefd[0]);
    Close(sockfd);
	Close(epfd);
  }
  else {
    Close(pipefd[1]);
  }
  return 0;
}
