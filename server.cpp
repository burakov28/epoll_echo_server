#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <string.h>
#include <set>
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <chrono>



using namespace std;


string connnection_established = "Connection established!\n";


const int EPOLL_SIZE = 20;
const int BUFFER_SIZE = 10;


int main() {
	int listener = socket(AF_INET, SOCK_STREAM, 0);
	if (fcntl(listener, F_SETFL, fcntl(listener, F_GETFD, 0) | O_NONBLOCK) < 0) {
		cerr << "Error while nonblocking socket" << endl;
		close(listener);
		return 0;
	}

	sockaddr_in my_addr;
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(8082);
	my_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(listener, (sockaddr *) &my_addr, sizeof(my_addr)) < 0) {
		cout << "Bind error" << endl;
		close(listener);
		return 0;
	}

	if (listen(listener, 128) < 0) {
		cout << "Set listener error" << endl;
		close(listener);
		return 0;
	}

	epoll_event ev, events[EPOLL_SIZE];

	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = listener;

	int epollfd;

	epollfd = epoll_create(EPOLL_SIZE);
	if (epollfd < 0) {
		cout << "Creating epoll error" << endl;
		close(listener);
		return 0;
	}

	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listener, &ev) < 0) {
		cout << "Adding socket to epoll error" << endl;
		close(listener);
		return 0;
	}

	sockaddr_in client_addr;
	socklen_t client_addr_len;
	char buffer[BUFFER_SIZE];

	set < int > clients;
	for (; ; ) {
		int events_number = epoll_wait(epollfd, events, EPOLL_SIZE, -1);
		if (events_number < 0) {
			this_thread::sleep_for(chrono::milliseconds(100));
			continue;
		}

		for (int i = 0; i < events_number; ++i) {
			if (events[i].data.fd == listener) {
				int client = accept(listener, (sockaddr *) &client_addr, &client_addr_len);
				//setnonblocking
				fcntl(client, F_SETFL, fcntl(client, F_GETFD, 0)| O_NONBLOCK);
				ev.data.fd = client;
				ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client, &ev) < 0) {
					cerr << "Error adding client. It was skipped" << endl;
					close(client);
					continue;	
				}

				

				clients.insert(client);
				write(client, connnection_established.c_str(), connnection_established.size());
			}
			else {
				int client = events[i].data.fd;
				memset(buffer, 0, sizeof(buffer));
				int was_read;
				while ((was_read = read(client, buffer, BUFFER_SIZE)) > 0) {
					for (auto fd : clients) {
						write(fd, buffer, was_read);
					}
				}

				if (was_read == 0) {	
					close(client);
					cerr << "client " << client << " close connection" << endl;
					clients.erase(client);
				}
			}
		}
	}
	close(listener);

	return 0;
}