#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <thread>
#include <fcntl.h>
#include <sys/epoll.h>


using namespace std;


const int BUFFER_SIZE = 10;
const int EPOLL_SIZE = 10;

struct writer {
	void operator() (int sock) {
		for (; ; ) {
			string s;
			getline(cin, s);

			write(sock, s.c_str(), s.size());

			cout << "I send message: " << s << endl;
		}
	}
};


char buffer[BUFFER_SIZE];



int main() {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(sock, F_SETFL, fcntl(sock, F_GETFD, 0)|O_NONBLOCK);
	hostent *server = gethostbyname("127.0.0.1");

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8082);
	memcpy(&addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);

	connect(sock, (sockaddr *) &addr, sizeof(addr));

	int epollfd = epoll_create(EPOLL_SIZE);

	epoll_event ev, events[EPOLL_SIZE];

	ev.data.fd = sock;
	ev.events = EPOLLIN | EPOLLET;

	epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ev);

	thread th_writer(writer(), sock);

	memset(buffer, 0, sizeof(buffer));

	for (; ; ) {
		int events_number = epoll_wait(epollfd, events, EPOLL_SIZE, -1);

		for (int i = 0; i < events_number; ++i) {
			int fd = events[i].data.fd;

			string s;
			int was_read;
			while ((was_read = read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
				s += string(buffer);
				memset(buffer, 0, sizeof(buffer));
			}
			if (was_read == 0) {
				cerr << "Server disconnected" << endl;
				return 0;
			}

			cout << "I got message: " << s << endl;
		}
	}

	
	return 0;
}