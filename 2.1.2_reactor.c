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

struct conn_item {
  int fd;
  char buffer[BUFFER_LENGTH];
  int idx;  // buffer当前占用了多少
};

struct conn_item conn_list[1024] = {0};

int main() {
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd == -1) return -1;

  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  // INADDR_ANY => 0.0.0.0
  servaddr.sin_port = htons(9999);

  int flag = bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
  if (flag == -1) return -2;

  listen(listenfd, 10);  // 10?

  int epfd = epoll_create(1);

  struct epoll_event ev, events[EVENTS_LENGTH];
  ev.events = EPOLLIN;
  ev.data.fd = listenfd;

  epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
  unsigned char buffer[BUFFER_LENGTH] = {0};
  while (1) {
    int nfds = epoll_wait(epfd, events, EVENTS_LENGTH, 0);
    for (int i = 0; i < nfds; i++) {
      int connfd = events[i].data.fd;
      if (listenfd == connfd) {
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        // accept什么时候阻塞，什么时候返回？
        int clientfd = accept(listenfd, (struct sockaddr *)&client, &len);

        // 复用了listenfd的ev,ev是临时变量？
        ev.events = EPOLLIN;  // 水平触发
        // ev.events = EPOLLIN | EPOLLET;  //边沿触发
        ev.data.fd = clientfd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);

        // init conn_item
        conn_list[clientfd].fd = clientfd;
        memset(conn_list[clientfd].buffer, 0, BUFFER_LENGTH);
        conn_list[clientfd].idx = 0;

        printf("clientfd:%d\n", clientfd);
      } else if (events[i].events & EPOLLIN)  //& means?
      {
        char *buffer = conn_list[connfd].buffer;
        int idx = conn_list[connfd].idx;

        int count = recv(connfd, buffer + idx, BUFFER_LENGTH - idx, 0);
        if (count == 0)  // disconnected
        {
          printf("client:%d disconnected\n", connfd);
          epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL);
          close(connfd);
        } else {
          // do sth with recived data
          printf("recv from clientfd:%d, count:%d, buffer:%s\n", connfd, count,
                 buffer);
          conn_list[connfd].idx += count;
          send(connfd, buffer, count, 0);
        }
      }
    }
  }
}
