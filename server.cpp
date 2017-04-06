#include "socket_wrapper.h"
#include "epoll_wrapper.h"
#include "timer_container.h"
#include "utils.h"
#include "logger.h"
#include <iostream>
#include <set>
#include <ctime>
#include <cassert>

using namespace std;

const unsigned long long CLIENT_TIMEOUT = 10; //seconds
const char *log_file = "server_log";

timer_container<socket_wrapper> timer;
map<socket_wrapper, timer_container<socket_wrapper>::iterator> in_timer;
set<socket_wrapper> clients;

void update_client(socket_wrapper const & client) {
	in_timer[client] = timer.modify(in_timer[client]);
}

void disconnect_client(socket_wrapper const & client) {
	clients.erase(client);
	timer.remove(in_timer[client]);
	in_timer.erase(client);
}

logger l;

void init_log() {
	l = logger(log_file);
}

void log(string message) {
	l.log(message);
}

int main() {
	init_log();

	socket_wrapper listener;
	log("Init server's socket: " + int_to_string(listener.get_fd()));

	listener.bind("0.0.0.0", 8082);
	log("Bind for all IPs on port 8082");

	listener.listen();
	log("Listen");

	listener.make_nonblock();
	log("Make server's socket nonblock");

	epoll_wrapper epoll;
	log("Init epoll");
	
	epoll.add(listener, EPOLLET | EPOLLIN);
	log("Add to epoll server's socket");
	log("");

	for (; ; ) {		
		while (!clients.empty() && (unsigned long long) time(NULL) >= timer.get_next_expiration()) {
			socket_wrapper tmp = timer.get_next().get_element();
			log("Close socket " + int_to_string(tmp.get_fd()) + " because client has exceeded silence's time limit");
			log("");
			disconnect_client(tmp);
		}

		vector <epoll_event> events;
		if (clients.empty()) {
			log("Epoll wait for the first client");
			events = epoll.wait();
		}
		else {
			unsigned long long ctime = time(NULL);
			assert(ctime < timer.get_next_expiration());
			log("Epoll wait any event for " + int_to_string(timer.get_next_expiration() - ctime) + " seconds");
			events = epoll.wait((timer.get_next_expiration() - ctime) * 1000); //in millis
		}
		log("");

		int sz = events.size();
		for (int i = 0; i < sz; ++i) {
			epoll_event cur = events[i];
			if (cur.data.fd == listener.get_fd()) {				
				socket_wrapper client = listener.accept();
				log("New client " + int_to_string(client.get_fd()) + " was connected");

				client.make_nonblock();
				log("Client " + int_to_string(client.get_fd()) + " was made nonblock");
				
				client.set_buffer();
				log("Init buffer for client " + int_to_string(client.get_fd()));
				
				epoll.add(client, EPOLLET | EPOLLIN);
				log("Client " + int_to_string(client.get_fd()) + " was added to epoll with writing only");

				clients.insert(client);
				in_timer[client] = timer.add(client, CLIENT_TIMEOUT);
				log("");
			}
			else {
				socket_wrapper client(cur.data.fd);
				auto it = clients.find(client);
				client.invalidate();
				client = *it;
				update_client(client);

				if (cur.events & EPOLLOUT) {
					log("Client " + int_to_string(client.get_fd()) + " ready for writing");
					client.write();
					if (client.empty()) {
						log("Client's buffer is empty. Added to epoll with writing only");
						epoll.modify(client, EPOLLET | EPOLLIN);	
					}
					log("");
				}
				if (cur.events & EPOLLIN) {
					log("Client " + int_to_string(client.get_fd()) + " ready for reading");	
					string message;
					if (client.read(message) == socket_wrapper::SOCKET_CLOSED) {
						log("Client " + int_to_string(client.get_fd()) + " was closed by foreign server");
						disconnect_client(client);						
					}
					else {
						log("Message: " + message + " was read");
						vector <socket_wrapper> bad;
						for (socket_wrapper client : clients) {
							if (client.write_to_buffer(message) == socket_wrapper::SOCKET_CLOSED) {
								bad.push_back(client);
							}
							else {
								log("Message has been written into buffer of client " + int_to_string(client.get_fd()));
								epoll.modify(client, EPOLLET | EPOLLIN | EPOLLOUT);
							}
						}

						for (socket_wrapper client : bad) {
							log("Client " + int_to_string(client.get_fd()) + " was closed because its buffer was overflowed");
							disconnect_client(client);
						}
					}
					log("");
				}
			}
		}
	}

	return 0;
}