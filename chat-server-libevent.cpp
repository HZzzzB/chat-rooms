// chat room server using libevent
// data: 2017.09.09

#include "np-header.h"

static const char MESSAGE[] = "Hello, World!\n";

static void listener_cb(struct evconnlistener *, evutil_socket_t,   // 
    struct sockaddr *, int socklen, void *);
static void conn_writecb(struct bufferevent *, void *);
static void conn_eventcb(struct bufferevent *, short, void *);
static void signal_cb(evutil_socket_t, short, void *);

/*
* 主函数创建用来监听连接的套接字， 然后创建 accept() 的回调函数来以便通过事件
* 处理函数处理每个连接
*/
int main(int argc, char **argv)
{
  struct event_base *base;  // 可以分配一个或多个 event_base 结构体。
                            // 每个 event_base 结构体持有一个事件集合， 可以检测以确定哪个事件是激活的。
                            // 每个 event_base 都有一种用于检测哪种事件已经就绪的方法或者后端
                            // 如： select, poll, epoll, kqueue, devpoll, evport, win32
  struct evconnlistener *listener;
  struct event *signal_event;

  struct sockaddr_in servaddr;

#ifdef _WIN32
  WSADATA wsa_data;
  WSAStartup(0x0201, &wsa_data);
#endif

  base = event_base_new();
  if (!base) {
    fprintf(stderr, "Could not initialize libevent!\n");
    return 1;
  }

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(SERV_IP);
  servaddr.sin_port = htons(SERV_PORT);

  listener = evconnlistener_new_bind(base, listener_cb, (void *)base,
      LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
      (struct sockaddr*)&servaddr,
      sizeof(servaddr));

  if (!listener) {
    fprintf(stderr, "Could not create a listener!\n");
    return 1;
  }

  signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);

  if (!signal_event || event_add(signal_event, NULL)<0) {
    fprintf(stderr, "Could not create/add a signal event!\n");
    return 1;
  }

  event_base_dispatch(base);

  evconnlistener_free(listener);
  event_free(signal_event);
  event_base_free(base);

  printf("done\n");
  return 0;
}

static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
    struct sockaddr *sa, int socklen, void *user_data) {
  struct event_base *base = user_data;
  struct bufferevent *bev;

  bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
  if (!bev) {
    fprintf(stderr, "Error constructing bufferevent!");
    event_base_loopbreak(base);
    return;
  }
  bufferevent_setcb(bev, NULL, conn_writecb, conn_eventcb, NULL);
  bufferevent_enable(bev, EV_WRITE);
  bufferevent_disable(bev, EV_READ);

  bufferevent_write(bev, MESSAGE, strlen(MESSAGE));
}

static void conn_writecb(struct bufferevent *bev, void *user_data) {
  struct evbuffer *output = bufferevent_get_output(bev);
  if (evbuffer_get_length(output) == 0) {
    printf("flushed answer\n");
    bufferevent_free(bev);
  }
}

static void
conn_eventcb(struct bufferevent *bev, short events, void *user_data) {
  if (events & BEV_EVENT_EOF) {
    printf("Connection closed.\n");
  } else if (events & BEV_EVENT_ERROR) {
    printf("Got an error on the connection: %s\n",
        strerror(errno));/*XXX win32*/
  }
  /* None of the other events can happen here, since we haven't enabled
   * timeouts */
  bufferevent_free(bev);
}

static void signal_cb(evutil_socket_t sig, short events, void *user_data) {
  struct event_base *base = user_data;
  struct timeval delay = { 2, 0 };

  printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");

  event_base_loopexit(base, &delay);
}
