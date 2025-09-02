#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#include <pthread.h>

#define BUFFER_LENGTH 128

void *routine(void *arg)
{
    int clientfd = *(int *)arg;

    while (1)
    {
        unsigned char buffer[BUFFER_LENGTH] = {0};
        int ret = recv(clientfd, buffer, BUFFER_LENGTH, 0);
        if (ret == 0)
        { //ret==0 即客户端断开连接
            close(clientfd);
            break;
        }
        printf("received buffer:%s, ret:%d \n", buffer, ret);
        send(clientfd, buffer, ret, 0);
    }
}
//
int main()
{

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
        return -1;

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //INADDR_ANY => 0.0.0.0
    servaddr.sin_port = htons(9999);

    int flag = bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (flag == -1)
        return -2;

    listen(listenfd, 10);

    /*单一客户端连接*/
    // struct sockaddr_in client;
    // socklen_t len = sizeof(client);
    // int clientfd = accept(listenfd, (struct sockaddr *)&client, &len);
    // printf("clientfd:%d\n", clientfd);

    // while (1)
    // {
    //     unsigned char buffer[BUFFER_LENGTH] = {0};
    //     int ret = recv(clientfd, buffer, BUFFER_LENGTH, 0);
    //     printf("received buffer:%s, ret:%d \n", buffer, ret);
    //     send(clientfd, buffer, ret, 0);
    // }

    /*多线程方法,1请求1线程*/
    // while (1)
    // {
    //     struct sockaddr_in client;
    //     socklen_t len = sizeof(client);
    //     int clientfd = accept(listenfd, (struct sockaddr *)&client, &len);
    //     printf("clientfd:%d\n", clientfd);

    //     pthread_t thread_id;
    //     pthread_create(&thread_id, NULL, routine, &clientfd);
    // }

    /*IO多路复用 select*/
    fd_set rfds, wfds, rset, wset;
    FD_ZERO(&rfds);
    FD_SET(listenfd, &rfds);

    FD_ZERO(&wfds);

    int maxfd = listenfd;

    int ret = 0;
    unsigned char buffer[BUFFER_LENGTH] = {0};

    while (1)
    {
        rset = rfds;
        wset = wfds;

        int nready = select(maxfd + 1, &rset, &wset, NULL, NULL);
        if (FD_ISSET(listenfd, &rset))
        {
            printf("listenfd:%d ready to read\n", listenfd); //listenfd ready to read
            struct sockaddr_in client;
            socklen_t len = sizeof(client);
            int clientfd = accept(listenfd, (struct sockaddr *)&client, &len);

            FD_SET(clientfd, &rfds);
            if (clientfd > maxfd)
                maxfd = clientfd;
        }

        int i = 0;
        for (i = listenfd + 1; i <= maxfd; i++)
        {

            if (FD_ISSET(i, &rset))
            {
                ret = recv(i, buffer, BUFFER_LENGTH, 0);
                if (ret == 0)
                { //ret==0 即客户端断开连接
                    close(i);
                    FD_CLR(i, &rfds);
                }
                else if (ret > 0)
                {
                    printf("received buffer:%s, ret:%d \n", buffer, ret);
                    FD_SET(i, &wfds);
                }
            }
            if (FD_ISSET(i, &wset))
            {
                send(i, buffer, ret, 0);
                FD_CLR(i, &wfds);
                FD_SET(i, &rfds);
            }
        }
    }
}