// chat client
// Fok
// Date: 2018.09.10

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <poll.h>
#include <fcntl.h>

#define BUFFER_SIZE 64

int main(int argc, char *argv[]) {
  if (argc <= 2) {
    printf("usage: %s IP_ADDRESS PORT\n", basename(argv[0]));
  exit(1);
  }

  const char *ip = argv[1];
  int port = atoi(argv[2]);

  int ret, sockfd;

  struct sockaddr_in serv_addr;
  bzero(&serv_addr, sizeof(serv_addr));
  
  serv_addr.sin_family = AF_INET;
  inet_pton(AF_INET, ip, &serv_addr.sin_addr);
  serv_addr.sin_port = htons(port);

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("client: error at socket");
  exit(1);
  }

  if ((ret = connect(sockfd, (struct sockaddr*) &serv_addr, 
      sizeof(serv_addr))) == -1) {
  close(sockfd);
    perror("client: error at connet");
  exit(1);
  }
  
  pollfd fds[2];

  fds[0].fd = 0;
  fds[0].events = POLLIN;
  fds[0].revents = 0;
  fds[1].fd = sockfd;
  fds[1].events = POLLIN | POLLRDHUP;
  fds[1].revents = 0;
  
  char read_buf[BUFFER_SIZE];
  int pipefd[2];
  if ((ret = pipe(pipefd)) == -1) {
  close(sockfd);
    perror("error at pipe");
  exit(1);
  }

  while (true) {
    if ((ret = poll(fds, 2, -1)) == -1) {
    perror("poll failed.\n");
    break;
  }

  if (fds[1].revents & POLLRDHUP) {
    close(sockfd);
    printf("server close the connection");
    exit(-1);
  }
    else if (fds[1].revents & POLLIN) {
    memset(read_buf, '\0', BUFFER_SIZE);
    recv(fds[1].fd, read_buf, BUFFER_SIZE - 1, 0);
    printf("%s\n", read_buf);
  }
    
  if (fds[0].revents & POLLIN) {
    ret = splice(0, NULL, pipefd[1], NULL, 32768,
               SPLICE_F_MORE | SPLICE_F_MOVE);
    ret = splice(pipefd[0], NULL, sockfd, NULL, 32768,
               SPLICE_F_MORE | SPLICE_F_MOVE);
  }

  }

  close(sockfd);

  return EXIT_SUCCESS;
}
