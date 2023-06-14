#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "bestline.h"

struct sockaddr_in serve;

int udp_open(int port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
    bzero(&serve, sizeof(serve));
    serve.sin_family = AF_INET;
    serve.sin_addr.s_addr = htonl(INADDR_ANY);
    serve.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *)&serve, sizeof(serve)) >= 0) {
        return sock;
    }
    return -1;
}

struct sockaddr_in client;
unsigned int client_len = sizeof(client);

int udp_read(int fd, void *c, int len) {
    int n = recvfrom(fd, c, len, 0, (struct sockaddr *)&client, &client_len);
    return n;
}

int sock = -1;
char sock_buf[1024];
int sock_count = 0;

int multi_read(int fd, void *c, int n) {
    struct pollfd p[2];
    p[0].fd = fd;
    p[0].events = POLLIN;
    p[1].fd = sock;
    p[1].events = POLLIN;
    int r = 0;
    char *m = (char *)c;
    while (1) {
        r = poll(p, 2, -1);
        if (r <= 0) {
            return r;
        }
        // check for sock events
        if (p[1].revents & POLLIN) {
            int u = udp_read(sock, sock_buf, sizeof(sock_buf));
            if (u > 0) {
                sock_count += u;
            }
        }
        // check for bestline events
        if (p[0].revents & POLLIN) {
            read(fd, m, n);
            m++;
            break;
        }
    }
    return r;
}

#define PORT (13961)

int main(int argc, char *argv[]) {
    char *l;
    sock = udp_open(PORT);
    if (sock >= 0) {
        printf("also listening on *:%d\n", PORT);
        bestlineUserIO(multi_read, NULL, NULL);
    }
    while (1) {
        l = bestline("> ");
        if (l == NULL) break;
        printf("GOT {%s} (%d in background)\n", l, sock_count);
        if (l[0] == '?') {
            for (int i=0; i<sizeof(sock_buf); i++) {
                if (sock_buf[i] == '\0') break;
                if (sock_buf[i] == '\n') {
                    sock_buf[i] = '\0';
                    break;
                }
            }
            printf("last background string was {%s}\n", sock_buf);
            sock_buf[0] = '\0';
        }
        sock_count = 0;
        free(l);
    }
    if (sock >= 0) {
        sock = -1;
        close(sock);
    }
    return 0;
}
