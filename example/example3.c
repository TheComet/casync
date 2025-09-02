#include "casync/casync.h"

#if defined(_WIN32)
#    define WIN32_LEAN_AND_MEAN
#    include "windows.h"
#    include "winsock2.h"
#    include "ws2tcpip.h"
#    define CLOSE_SOCKET(x) closesocket(x)
#    define SHUT_RDWR       SD_BOTH
#else
#    include <arpa/inet.h>
#    include <fcntl.h>
#    include <netdb.h>
#    include <netinet/in.h>
#    include <sys/socket.h>
#    include <unistd.h>
#    define CLOSE_SOCKET(x) close(x)
#endif

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define PORT    "8080"
#define BACKLOG 10

#if defined(_MSC_VER)
#    define ALIGNED(x)
#else
#    define ALIGNED(x) __attribute__((aligned(x)))
#endif

struct end_server_args
{
    int timeout_ms;
    int server_fd;
};

static uint32_t stacks[1024 * 64][10] ALIGNED(16);

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
    char          enable = 1;
    if (ioctlsocket(sockfd, FIONBIO, &nonblock) != 0)
        return log_err("ioctlsocket() failed: %d\n", WSAGetLastError());
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(char)) < 0)
        return log_err(
            "setsocketopt(SO_REUSEADDR) failed: %d\n", WSAGetLastError());
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

static int async_accept(
    int fd, struct sockaddr_storage* addr, socklen_t* __restrict addr_len)
{
    char s[INET6_ADDRSTRLEN];

    while (1)
    {
        int new_fd = accept(fd, (struct sockaddr*)addr, addr_len);
        if (new_fd == -1 &&
#if defined(_WIN32)
            (WSAGetLastError() == WSAEWOULDBLOCK ||
             WSAGetLastError() == WSAEINPROGRESS))
#else
            (errno == EAGAIN || errno == EWOULDBLOCK))
#endif
        {
            casync_yield();
            continue;
        }

        if (new_fd != -1)
        {
            inet_ntop(
                addr->ss_family,
                &((struct sockaddr_in*)addr)->sin_addr,
                s,
                sizeof s);
            fprintf(stderr, "server: got connection from %s\n", s);
        }

        return new_fd;
    }
}

static int async_recv(int fd, void* buf, size_t n, int flags)
{
    while (1)
    {
        int rc = recv(fd, buf, n, flags);
        if (rc == -1 &&
#if defined(_WIN32)
            (WSAGetLastError() == WSAEWOULDBLOCK ||
             WSAGetLastError() == WSAEINPROGRESS))
#else
            (errno == EWOULDBLOCK || errno == EAGAIN))
#endif
        {
            casync_yield();
            continue;
        }

        if (rc == -1)
            log_err("client: recv() failed: %s\n", strerror(errno));
        return rc;
    }
}

static int handle_client(void* arg)
{
    char buf[64];
    int  rc;
    int  fd = (int)(intptr_t)arg;

    while (1)
    {
        rc = async_recv(fd, buf, 64 - 1, 0);
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
    struct end_server_args* args = arg;
    casync_sleep_ms(args->timeout_ms);
    fprintf(stderr, "stopping server...\n");
    shutdown(args->server_fd, SHUT_RDWR);
    return 0;
}

static int server(void* arg)
{
    int                     sockfd, new_fd;
    struct addrinfo         hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t               sin_size;
    int                     rc;
    struct end_server_args  end_server_args;
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

    end_server_args.timeout_ms = 200;
    end_server_args.server_fd = sockfd;
    casync_start_static(end_server_timer, &end_server_args);

    while (1)
    {
        sin_size = sizeof their_addr;
        new_fd = async_accept(sockfd, &their_addr, &sin_size);
        if (new_fd == -1)
            break;

        if (set_nonblock_reuse(new_fd) != 0)
        {
            CLOSE_SOCKET(new_fd);
            continue;
        }

        casync_start_static(handle_client, (void*)(intptr_t)new_fd);
    }

    CLOSE_SOCKET(sockfd);

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

    rc = async_recv(sockfd, buf, 64 - 1, 0);
    if (rc > 0)
    {
        buf[rc] = '\0';
        fprintf(stderr, "client: received '%s'\n", buf);
    }

    fprintf(stderr, "client: Disconnecting...\n");
    CLOSE_SOCKET(sockfd);

    return 0;
}

int main(void)
{
#if defined(_WIN32)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return log_err("WSAStartup failed\n");
    if (LOBYTE(wsaData.wVersion) != 2)
        return log_err("Version 2.x of Winsock is not available\n");
#endif

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

#if defined(_WIN32)
    WSACleanup();
#endif

    return 0;
}
