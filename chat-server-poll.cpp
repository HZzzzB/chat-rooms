// chat room server using poll
// Date: 2017.09.09

#include "np-header.h"

#define BACKLOG 512

#define WELCOME "Welcome to the chat room! Here you can chat freely!"
#define FORMAT "user %d >> %s"

struct client_data {
  struct sockaddr_in addr;
  char *write_buf;
  char buf[BUFFSIZE];
};

int main(int argc, char *argv[]) {
  
  int listenfd(0), connfd, ret(0);
  struct sockaddr_in servaddr;

  bzero(&servaddr, sizeof(servaddr));

  servaddr.sin_family = AF_INET;
  inet_pton(AF_INET, SERV_IP, &servaddr.sin_addr);
  servaddr.sin_port = htons(SERV_PORT);

  if ((listenfd = Socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("server: error at socket.");
	exit(1);
  }

  if ((ret = Bind(listenfd, (struct sockaddr*) &servaddr, 
		  sizeof(servaddr))) == -1) {
    Close(listenfd);
	exit(1);
  }

  Listen(listenfd, BACKLOG);
  printf("chat room server start...\n");

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
		setnonblock(connfd);
		fds[user_counter].fd = connfd;
		fds[user_counter].events = POLLIN | POLLRDHUP | POLLERR;
		fds[user_counter].revents = 0;

		printf("come a new user, now have %d users\n", user_counter);
        
		Send(connfd, WELCOME, BUFFSIZE, 0);
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
		printf("user %d left the chat room\n", fds[i].fd);
	    users[fds[i].fd] = users[fds[user_counter].fd];
		close(fds[i].fd);
		--i;
		--user_counter;
	  }
	  else if (fds[i].revents & POLLIN) {
	    connfd = fds[i].fd;
		memset(users[connfd].buf, '\0', BUFFSIZE);
		ret = recv(connfd, users[connfd].buf, BUFFSIZE, 0);
		printf("get data from user %d\n", connfd);
		if (ret < 0) {
		  if (errno != EAGAIN) {
		    close(connfd);
			users[fds[i].fd] = users[fds[user_counter].fd];
			fds[i] = fds[user_counter];
			--i;
			--user_counter;
		  }
		}
		else if (ret == 0) {  // connection close normally 
          printf("user %d left the chat room...\n", fds[i].fd);
		  Close(connfd);
		  users[fds[i].fd] = users[fds[user_counter].fd];
		  fds[i] = fds[user_counter];
		  --i;
		  --user_counter;
		} 
		else {
		  for(int j = 1; j <= user_counter; ++j) {
			if (fds[j].fd == connfd) {
			  continue;
			}

			fds[j].events |= ~POLLIN;
			fds[j].events |= POLLOUT;
			char message[BUFFSIZE];
			bzero(&message, BUFFSIZE);
			sprintf(message, FORMAT, connfd, users[connfd].buf);
			users[fds[j].fd].write_buf = message;
		  }
		}
	  }
	  else if (fds[i].revents & POLLOUT) {
		connfd = fds[i].fd;
		if (!users[connfd].write_buf) {
		  continue;
		}
		ret = send(connfd, users[connfd].write_buf, BUFFSIZE, 0);
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
