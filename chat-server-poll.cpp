// Chat server 
// Copyright, hzzzzb213@gmail.com, Fok.
// Date: 2017.09.09

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>

#include <assert.h>
#include <unistd.h>
#include <errno.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <poll.h>
#include <fcntl.h>

#define PORT 9988

#define BACKLOG 5 // how many pending connections 
#define BUFFER_SIZE 64 // Size of Read Buffer
#define FD_LIMIT 65535 // FD numbers limit

struct client_data {
  sockaddr_in addr;
  char *write_buf;
  char buf[BUFFER_SIZE];
};

int setnonblocking(int fd) {
  int old_opt = fcntl(fd, F_GETFL);
  int new_opt = old_opt | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_opt);
  return old_opt;
}

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    printf("usage: %s IP_ADDRESS\n", basename(argv[0]));
  exit(1);
  }
  
  const char *ip = argv[1];
  int listenfd(0), connfd, ret(0);
  struct sockaddr_in addr;

  bzero(&addr, sizeof(addr));

  addr.sin_family = AF_INET;
  inet_pton(AF_INET, ip, &addr.sin_addr);
  addr.sin_port = htons(PORT);

  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("server: error at socket.");
  exit(1);
  }

  if ((ret = bind(listenfd, (struct sockaddr*) &addr, sizeof(addr))) == -1) {
    close(listenfd);
  perror("server: error at bind.");
  exit(1);
  }

  if ((ret = listen(listenfd, 5))) {
    perror("server: error at listen.");
  exit(1);
  }

  client_data *users = new client_data[FD_LIMIT];

  pollfd fds[BACKLOG + 1];
  int user_counter = 0;
  for (int i = 1; i <= BACKLOG; ++i) {
    fds[i].fd = -1;
  fds[i].events = 0;
  }
  fds[0].fd = listenfd;
  fds[0].events = POLLIN | POLLERR;
  fds[0].revents = 0;

  while (true) {
    if ((ret = poll(fds, user_counter + 1, -1)) == -1) {
    perror("server: error at poll");
    exit(1);
  }

  for (int i = 0; i < user_counter + 1; ++i) {
    if ((fds[i].fd == listenfd) && (fds[i].revents && POLLIN)) {
      struct sockaddr_in cli_addr;
    socklen_t cli_addr_len = sizeof(cli_addr);
    if ((connfd = accept(listenfd, (struct sockaddr*) &cli_addr,
        &cli_addr_len)) == -1) {
          perror("server: error at accept");
      continue;
    }

    if (user_counter >= BACKLOG) {
      const char *info = "too many users.\n";
      perror(info);
      send(connfd, info, strlen(info), 0);
      close(connfd);
      continue;
    }

    ++user_counter;
        users[connfd].addr = cli_addr;
    setnonblocking(connfd);
    fds[user_counter].fd = connfd;
    fds[user_counter].events = POLLIN | POLLRDHUP | POLLERR;
    fds[user_counter].revents = 0;

    printf("come a new user, now have %d users\n", user_counter);
    }
    else if (fds[i].revents & POLLERR) {
      printf("get an error from %d\n", fds[i].fd);
    char errors[100];
    memset(errors, '\0', 100);
    socklen_t len = sizeof(errors);
    if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, 
        &len) < 0) {
      perror("get socket option failed");
    }
    continue;
    }
      else if (fds[i].revents & POLLRDHUP) {
      users[fds[i].fd] = users[fds[user_counter].fd];
    close(fds[i].fd);
    --i;
    --user_counter;
    printf("a client left\n");
    }
    else if (fds[i].revents & POLLIN) {
      connfd = fds[i].fd;
    memset(users[connfd].buf, '\0', BUFFER_SIZE);
    ret = recv(connfd, users[connfd].buf, BUFFER_SIZE - 1, 0);
    printf("get %d bytes of client data %s from %d.\n", ret,
          users[connfd].buf, connfd);
    if (ret < 0) {
      if (errno != EAGAIN) {
        close(connfd);
      users[fds[i].fd] = users[fds[user_counter].fd];
      fds[i] = fds[user_counter];
      --i;
      --user_counter;
      }
    }
    else if (ret == 0) {
    } 
    else {
      for(int j = 1; j <= user_counter; ++j) {
      if (fds[j].fd == connfd) {
        continue;
      }

      fds[j].events |= ~POLLIN;
      fds[j].events |= POLLOUT;
      users[fds[j].fd].write_buf = users[connfd].buf;
      }
    }
    }
    else if (fds[i].revents & POLLOUT) {
    connfd = fds[i].fd;
    if (!users[connfd].write_buf) {
      continue;
    }
    ret = send(connfd, users[connfd].write_buf, strlen(users[connfd].write_buf), 0);
    users[connfd].write_buf = NULL;

    fds[i].events |= ~POLLOUT;
    fds[i].events |= POLLIN;
      }
  }
  }

  delete[] users;
  close(listenfd);
  
  return EXIT_SUCCESS;
}
