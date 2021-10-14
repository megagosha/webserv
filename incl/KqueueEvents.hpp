//
// Created by Elayne Debi on 9/8/21.
//

#ifndef KQUEUE_EVENTS_HPP
#define KQUEUE_EVENTS_HPP

#include <set>
#include <sys/event.h>
#include <cerrno>

class KqueueEvents
{
private:
	const int _max_size;
	int _queue_fd;
	std::set<int> _fds;
	struct kevent *_w_event;
	struct kevent *_res_event;

	KqueueEvents();

public:
	explicit KqueueEvents(int max_size);

	KqueueEvents(int max_size, int connection_socket);

	void init(void);

	KqueueEvents(KqueueEvents const &ke);

	KqueueEvents &operator=(KqueueEvents const &ke);

	~KqueueEvents();

	void addFd(int fd, bool write = false);

	void addProcess(pid_t proc);

	void deleteFd(int fd, bool write = false);

	std::pair<int, struct kevent *> getUpdates(int = 5);

	class KqueueException : public std::exception
	{
		virtual const char *what() const _NOEXCEPT;
	};
};

#endif //UNTITLED_KQUEUEEVENTS_HPP
