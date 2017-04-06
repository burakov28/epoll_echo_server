#include <sys/epoll.h>
#include <vector>
#include "socket_wrapper.h"


struct epoll_wrapper {
	static const int MAXEVENTS = 20;

	int epollfd;	
	epoll_event events[MAXEVENTS];

	epoll_wrapper(int epoll_size = 10) {
		epollfd = epoll_create(epoll_size);
	}

	epoll_wrapper(epoll_wrapper const &) = delete;
	epoll_wrapper(epoll_wrapper &) = delete;

	~epoll_wrapper() {
		::close(epollfd);
	}

	std::vector <epoll_event> wait(int timeout = -1) {
		int fds = epoll_wait(epollfd, events, MAXEVENTS, timeout);
		std::vector <epoll_event> ret;
		for (int i = 0; i < fds; ++i) {
			ret.push_back(events[i]);
		}
		return ret;
	}
  

	void add(socket_wrapper const & socket, uint32_t events) {
		epoll_event ev;
		memset(&ev, 0, sizeof(ev));
		ev.events = events;
		ev.data.fd = socket.get_fd();	
		epoll_ctl(epollfd, EPOLL_CTL_ADD, socket.get_fd(), &ev);
	}

	void modify(socket_wrapper const & socket, uint32_t events) {
		epoll_event ev;
		epoll_ctl(epollfd, EPOLL_CTL_DEL, socket.get_fd(), NULL);
		memset(&ev, 0, sizeof(ev));
		ev.events = events;
		ev.data.fd = socket.get_fd();
		epoll_ctl(epollfd, EPOLL_CTL_ADD, socket.get_fd(), &ev);
	}

	void close() {
		::close(epollfd);
	}
};