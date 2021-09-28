#byte order converters
include <arpa/inet.h>

    uint32_t htonl(uint32_t hostlong);
    uint16_t htons(uint16_t hostshort);
    uint32_t ntohl(uint32_t netlong);
    uint16_t ntohs(uint16_t netshort);

The htons() function converts the unsigned short integer hostshort from host byte order to network byte order.

The ntohl() function converts the unsigned integer netlong from network byte order to host byte order.

The ntohs() function converts the unsigned short integer netshort from network byte order to host byte order.

# select
    int select(int nfds, fd_set *restrict readfds,
                  fd_set *restrict writefds, fd_set *restrict exceptfds,
                  struct timeval *restrict timeout);
    void FD_CLR(int fd, fd_set *set);
    int  FD_ISSET(int fd, fd_set *set); 
    void FD_SET(int fd, fd_set *set);
    void FD_ZERO(fd_set *set);

**WARNING**: select() can monitor only file descriptors numbers that
are less than FD_SETSIZE (1024)—an unreasonably low limit for
many modern applications—and this limitation will not change.
All modern applications should instead use poll(2) or epoll(7),
which do not suffer this limitation.

select() allows a program to monitor multiple file descriptors,
waiting until one or more of the file descriptors become "ready"
for some class of I/O operation (e.g., input possible).  A file
descriptor is considered ready if it is possible to perform a
corresponding I/O operation (e.g., read(2), or a sufficiently
small write(2)) without blocking.


#poll
poll, ppoll - wait for some event on a file descriptor

include <poll.h>

       int poll(struct pollfd *fds, nfds_t nfds, int timeout);

       #define _GNU_SOURCE         /* See feature_test_macros(7) */
       #include <poll.h>

       int ppoll(struct pollfd *fds, nfds_t nfds,
                 const struct timespec *tmo_p, const sigset_t *sigmask);

epoll (epoll_create, epoll_ctl,
epoll_wait)
kqueue (kqueue, kevent)
socket,
accept,
listen,
send,
recv,
bind,
connect,
inet_addr,
setsockopt,
getsockname,
fcntl.# webserv
