//
// Created by Elayne Debi on 9/8/21.
//

#ifndef UNTITLED_KQUEUEEVENTS_HPP
#define UNTITLED_KQUEUEEVENTS_HPP

#include <set>
#include <sys/event.h>
#include <cerrno>

class KqueueEvents {
private:
    const int _max_size;
    int _queue_fd;
    std::set<int> _fds;
    struct kevent *_w_event;
    struct kevent *_res_event;


public:
    KqueueEvents(int max_size, int connection_socket) : _max_size(max_size) {
        _fds.insert(connection_socket);
        _queue_fd = kqueue();
        if (_queue_fd == -1)
            throw KqueueException();
        _w_event = new struct kevent;
        _res_event = new struct kevent[_max_size];
        EV_SET(_w_event, connection_socket, EVFILT_READ, EV_ADD, 0, 0, NULL);
        if (kevent(_queue_fd, _w_event, 1, NULL, 0, NULL) == -1)
            throw KqueueException();
    }

    void init(void) {
        _queue_fd = kqueue();
        if (_queue_fd == -1)
            throw KqueueException();
    }

    KqueueEvents(KqueueEvents const &ke) : _max_size(ke._max_size) {
        _queue_fd = ke._queue_fd;
        _fds = ke._fds;
        _w_event = new struct kevent;
        _res_event = new struct kevent[_max_size];
        _w_event = ke._w_event;
        _res_event  = ke._res_event;
    }

    KqueueEvents &operator=(KqueueEvents const &ke) {
        if (this == &ke)
            return (*this);
        if (_max_size != ke._max_size)
            throw KqueueException();
        _queue_fd = ke._queue_fd;
        _fds = ke._fds;
        _w_event = ke._w_event;
        _w_event = ke._res_event;
        return (*this);
    }

    ~KqueueEvents() {
        delete _w_event;
        delete [] _res_event;
    }

    void addFd(int fd, bool write = false) {
        _fds.insert(fd);
        EV_SET(_w_event, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
        if (kevent(_queue_fd, _w_event, 1, NULL, 0, NULL) == -1)
            throw KqueueException();
        if (write) {
            EV_SET(_w_event, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
            if (kevent(_queue_fd, _w_event, 1, NULL, 0, NULL) == -1)
                throw KqueueException();
        }
    }

    void deleteFd(int fd, bool write = false) {
        _fds.erase(fd);
        EV_SET(_w_event, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
        kevent(_queue_fd, _w_event, 1, NULL, 0, NULL);
        if (write) {
            EV_SET(_w_event, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
            kevent(_queue_fd, _w_event, 1, NULL, 0, NULL);
        }
    }

    std::pair<int, struct kevent *> getUpdates(void) {
        struct timespec tmout = {5,     /* block for 5 seconds at most */
                                 0};
        int res = kevent(_queue_fd, NULL, 0, _res_event, _max_size, &tmout);
        return (std::make_pair(res, _res_event));
    }

    class KqueueException : public std::exception {
        virtual const char *what() throw() {
            return (std::strerror(errno));
        }
    };
};

#endif //UNTITLED_KQUEUEEVENTS_HPP
