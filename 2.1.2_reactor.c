#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#define BUFFER_LENGTH 128
#define EVENTS_LENGTH 128

/*reactor 模型*/
/*epoll 面向io的写法改为面向事件 */

// event callback
int accept_cb(int fd);
int recv_cb(int fd);
int send_cb(int fd);

typedef int (*RCALLBACK) (int fd); //grama

struct conn_item {
  int fd;
  char buffer[BUFFER_LENGTH];
  int idx;  // buffer当前占用了多少

  RCALLBACK accept_callback;
  RCALLBACK recv_callback; 
  RCALLBACK send_callback;

};

struct conn_item connlist[1024] = {0};

int epfd = 0;

void set_event(int fd, uint32_t event, int flag) {
  // flag ==1 => add event; flag==0 => modify event
  if (flag) {
    struct epoll_event ev;
    ev.events = event;  // 水平触发
    // ev.events = EPOLLIN | EPOLLET;  //边沿触发
    ev.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
  } else {
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
  }
}

int accept_cb(int listenfd) {
  struct sockaddr_in client;
  socklen_t len = sizeof(client);
  // accept什么时候阻塞，什么时候返回？
  int clientfd = accept(listenfd, (struct sockaddr *)&client, &len);
  if (clientfd < 0) return -1;

  set_event(clientfd, EPOLLIN, 1);

  // init conn_item
  connlist[clientfd].fd = clientfd;
  memset(connlist[clientfd].buffer, 0, BUFFER_LENGTH);
  connlist[clientfd].idx = 0;
  connlist[clientfd].recv_callback=recv_cb;
  connlist[clientfd].send_callback=send_cb;

  printf("clientfd:%d\n", clientfd);

  return clientfd;
}

int recv_cb(int fd) {
  char *buffer = connlist[fd].buffer;
  int idx = connlist[fd].idx;

  int count = recv(fd, buffer + idx, BUFFER_LENGTH - idx, 0);
  if (count == 0)  // disconnected
  {
    printf("client:%d disconnected\n", fd);
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
    return -1;
  }
  connlist[fd].idx += count;

  // update event
  set_event(fd, EPOLLOUT, 0);

  return count;
}

int send_cb(int fd) {
  char *buffer = connlist[fd].buffer;
  int idx = connlist[fd].idx;
  int count = send(fd, buffer, idx, 0);

  // update event
  set_event(fd, EPOLLIN, 0);

  return count;
}

int main() {
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd == -1) return -1;

  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  // INADDR_ANY => 0.0.0.0
  servaddr.sin_port = htons(9999);

  int flag = bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
  if (flag == -1) return -2;

  listen(listenfd, 10);  // 10 means?

  epfd = epoll_create(1);

  set_event(listenfd, EPOLLIN, 1);
  struct epoll_event events[EVENTS_LENGTH] = {0};

  while (1) {
    int nfds = epoll_wait(epfd, events, EVENTS_LENGTH, 0);
    for (int i = 0; i < nfds; i++) {
      int connfd = events[i].data.fd;
      if (listenfd == connfd) {
        accept_cb(connfd);
      } else if (events[i].events & EPOLLIN)  //& means?
      {
        int count=connlist[connfd].recv_callback(connfd);
        printf("recv <-- buffer:%s\n", connlist[connfd].buffer);
      } else if (events[i].events & EPOLLOUT) {
        int count=connlist[connfd].send_callback(connfd);
        printf("send --> buffer:%s\n", connlist[connfd].buffer);
      }
    }
  }
}
