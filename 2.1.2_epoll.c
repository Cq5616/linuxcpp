#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/epoll.h>

#define BUFFER_LENGTH 128
#define EVENTS_LENGTH 128

/*IO多路复用 epoll api*/
int main()
{

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
        return -1;

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY => 0.0.0.0
    servaddr.sin_port = htons(9999);

    int flag = bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (flag == -1)
        return -2;

    listen(listenfd, 10);

    int epfd = epoll_create(1);

    struct epoll_event ev, events[EVENTS_LENGTH];
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;

    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
    unsigned char buffer[BUFFER_LENGTH] = {0};
    while (1)
    {
        int nfds = epoll_wait(epfd, events, EVENTS_LENGTH, 0);
        for (int i = 0; i < nfds; i++)
        {
            if (events[i].events & EPOLLIN)
            {
                if (events[i].data.fd == listenfd)
                {
                    // new conn
                    struct sockaddr_in client;
                    socklen_t len = sizeof(client);
                    int clientfd = accept(listenfd, (struct sockaddr *)&client, &len);

                    epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);

                    printf("event_fd:%d", events[i].data.fd);
                }

                // {
                //     printf("listenfd:%d ready to read\n", listenfd); // listenfd ready to read
                //     struct sockaddr_in client;
                //     socklen_t len = sizeof(client);
                //     int clientfd = accept(listenfd, (struct sockaddr *)&client, &len);

                //     FD_SET(clientfd, &rfds);
                //     if (clientfd > maxfd)
                //         maxfd = clientfd;
                // }
            }
        }
    }
}