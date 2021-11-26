//
// Created by Elayne Debi on 9/8/21.
//

#include "KqueueEvents.hpp"
#include <iostream>

KqueueEvents::KqueueEvents() : _max_size(0) {}

KqueueEvents::KqueueEvents(int max_size) : _max_size(max_size) {
    _queue_fd = kqueue();
    if (_queue_fd == -1)
        throw KqueueException();
    _w_event   = new struct kevent;
    _res_event = new struct kevent[_max_size];
}

KqueueEvents::KqueueEvents(KqueueEvents const &ke) : _max_size(ke._max_size) {
    _queue_fd  = ke._queue_fd;
    _fds       = ke._fds;
    _w_event   = new struct kevent;
    _res_event = new struct kevent[_max_size];
    _w_event   = ke._w_event;
    _res_event = ke._res_event;
}

KqueueEvents &KqueueEvents::operator=(KqueueEvents const &ke) {
    if (this == &ke)
        return (*this);
    if (_max_size != ke._max_size)
        throw KqueueException();
    _queue_fd = ke._queue_fd;
    _fds      = ke._fds;
    _w_event  = ke._w_event;
    _w_event  = ke._res_event;
    return (*this);
}

KqueueEvents::~KqueueEvents() {
    delete _w_event;
    delete[] _res_event;
}

void KqueueEvents::addFd(int fd, short type) {
    _fds.insert(fd);

    uint64_t t_fd   = fd;
    uint32_t fflags = 0;
    if (type == EVFILT_PROC)
        fflags = NOTE_EXIT;
    EV_SET(_w_event, t_fd, type, EV_ADD, fflags, 0, NULL);
    if (kevent(_queue_fd, _w_event, 1, nullptr, 0, nullptr) == -1) {
        std::cout << strerror(errno) << std::endl;
        throw KqueueException();
    }
}

void KqueueEvents::deleteFd(int fd, short type) {
    _fds.erase(fd);
    EV_SET(_w_event, fd, type, EV_DELETE, 0, 0, NULL);
    if (kevent(_queue_fd, _w_event, 1, nullptr, 0, nullptr) == -1) {
        if (type != EVFILT_PROC)
            throw KqueueException();
    }

}

std::pair<int, struct kevent *> KqueueEvents::getUpdates(int tout) {
    struct timespec tmout = {tout,     /* block for 5 seconds at most */
                             0};
    int             res   = kevent(_queue_fd, nullptr, 0, _res_event, _max_size, &tmout);
    return (std::make_pair(res, _res_event));
}

const char *KqueueEvents::KqueueException::what() const _NOEXCEPT {
    return (std::strerror(errno));
}
