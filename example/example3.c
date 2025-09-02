#include "casync/casync.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#define CLOSE_SOCKET(x) closesocket(x)
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define CLOSE_SOCKET(x) close(x)
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define PORT    "8080"
#define BACKLOG 10

#if defined(_MSC_VER)
#   define ALIGNED(x)
#else
#   define ALIGNED(x) __attribute__((aligned(x)))
#endif

static int stop_server;

static int log_err(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    return -1;
}

static int set_nonblock_reuse(int sockfd)
{
#if defined(_WIN32)
    unsigned long nonblock = 1;
    char enable = 1;
    if (ioctlsocket(sockfd, FIONBIO, &nonblock) != 0)
        return log_err("ioctlsocket() failed: %d\n", WSAGetLastError());
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(char)) < 0)
        return log_err("setsocketopt(SO_REUSEADDR) failed: %d\n", WSAGetLastError());
#else
    const int enable = 1;
    int       flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1)
        return log_err("fcntl() failed for socket: %s\n", strerror(errno));
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0)
        return log_err("fcntl() failed for socket: %s\n", strerror(errno));
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        return log_err(
            "sotsocketopt(SO_REUSEADDR) failed for socket: %s\n",
            strerror(errno));
#endif

    return 0;
}

static int handle_client(void* arg)
{
    char buf[64];
    int  rc;
    int  fd = (int)(intptr_t)arg;

    while (1)
    {
        while ((rc = recv(fd, buf, 64 - 1, 0)) == -1 &&
#if defined(_WIN32)
            (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINPROGRESS))
#else
               (errno == EWOULDBLOCK || errno == EAGAIN))
#endif
        {
            casync_yield();
        }

        if (rc < 0)
            log_err("server: recv() failed: %s\n", strerror(errno));
        if (rc <= 0)
            break;

        buf[rc] = '\0';
        fprintf(stderr, "server: Echoing message '%s'\n", buf);
        send(fd, buf, rc, 0);
    }

    fprintf(stderr, "server: Client disconnected\n");
    CLOSE_SOCKET(fd);
    return 0;
}

static int end_server_timer(void* arg)
{
    int timeout_ms = (int)(intptr_t)arg;
    casync_sleep_ms(timeout_ms);
    fprintf(stderr, "stopping server...\n");
    stop_server = 1;
    return 0;
}

static int server(void* arg)
{
    int                     sockfd, new_fd;
    struct addrinfo         hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t               sin_size;
    char                    s[INET6_ADDRSTRLEN];
    int                     rc;
    (void)arg;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rc = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
        return log_err("getaddrinfo: %s\n", gai_strerror(rc));

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
            return log_err("socket() failed: %s\n", strerror(errno));
        if (set_nonblock_reuse(sockfd) != 0)
            return -1;
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            CLOSE_SOCKET(sockfd);
            log_err("bind() failed: %s\n", strerror(errno));
            continue;
        }
        break;
    }
    freeaddrinfo(servinfo);

    if (p == NULL)
        return log_err("No socket bound.\n");

    if (listen(sockfd, BACKLOG) == -1)
        return log_err("listen() failed: %s\n", strerror(errno));

    casync_start_static(end_server_timer, (void*)(intptr_t)200);

    while (stop_server == 0)
    {
        sin_size = sizeof their_addr;
        if ((new_fd = accept(
                 sockfd, (struct sockaddr*)&their_addr, &sin_size)) == -1 &&
#if defined(_WIN32)
            (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINPROGRESS))
#else
            (errno == EAGAIN || errno == EWOULDBLOCK))
#endif
        {
            casync_yield();
            continue;
        }

        if (new_fd == -1)
            return log_err("accept failed(): %s\n", strerror(errno));
        if (set_nonblock_reuse(new_fd) != 0)
            return -1;

        inet_ntop(
            their_addr.ss_family,
            &((struct sockaddr_in*)&their_addr)->sin_addr,
            s,
            sizeof s);
        fprintf(stderr, "server: got connection from %s\n", s);

        casync_start_static(handle_client, (void*)(intptr_t)new_fd);
    }

    return 0;
}

static int client(void* arg)
{
    int             sockfd;
    char            buf[64];
    struct addrinfo hints, *servinfo, *p;
    int             rc, msg_len;
    (void)arg;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((rc = getaddrinfo("localhost", PORT, &hints, &servinfo)) != 0)
        return log_err("client: getaddrinfo: %s\n", gai_strerror(rc));

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
            return log_err("client: socket() failed: %s\n", strerror(errno));
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) != 0)
        {
            log_err("client: connect() failed: %s\n", strerror(errno));
            CLOSE_SOCKET(sockfd);
            continue;
        }
        if (set_nonblock_reuse(sockfd) != 0)
        {
            CLOSE_SOCKET(sockfd);
            continue;
        }
        break;
    }
    freeaddrinfo(servinfo);

    if (p == NULL)
        return log_err("client: Failed to connect\n");

    msg_len = sprintf(buf, "Hello from %d", sockfd);
    buf[msg_len] = '\0';
    fprintf(stderr, "client: Sending '%s'\n", buf);
    send(sockfd, buf, msg_len, 0);

    while ((rc = recv(sockfd, buf, 64 - 1, 0)) == -1 &&
#if defined(_WIN32)
            (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINPROGRESS))
#else
           (errno == EWOULDBLOCK || errno == EAGAIN))
#endif
    {
        casync_yield();
    }
    if (rc > 0)
    {
        buf[rc] = '\0';
        fprintf(stderr, "client: received '%s'\n", buf);
    }
    else
        log_err("client: recv() failed: %s\n", strerror(errno));

    fprintf(stderr, "client: Disconnecting...\n");
    CLOSE_SOCKET(sockfd);

    return 0;
}

static uint32_t stacks[1024 * 64][10] ALIGNED(16);

int main(void)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return log_err("WSAStartup failed\n");
    if (LOBYTE(wsaData.wVersion) != 2)
        return log_err("Version 2.x of Winsock is not available\n");

    /* clang-format off */
    casync_gather_static(
        casync_stack_pool_init_linear(stacks,
            sizeof(stacks)/sizeof(*stacks),
            sizeof(*stacks)/sizeof(**stacks)),
        5,
        server, NULL,
        client, NULL,
        client, NULL,
        client, NULL,
        client, NULL);
    /* clang-format on */

    WSACleanup();

    return 0;
}
