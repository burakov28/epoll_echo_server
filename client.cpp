#include <iostream>
#include "socket_wrapper.h"
#include "epoll_wrapper.h"
#include "logger.h"
#include "timer_container.h"
#include "utils.h"
#include <thread>


using namespace std;

const char *log_file = "client_log";

logger l;

void init_log() {
	l = logger(log_file);
}

void log(string message) {
	l.log(message);
}

void server_disconnected() {
	cerr << "Server was disconnected!" << endl;
	exit(0);
}

string output_buffer;

void flush_buffer() {
	cout << output_buffer << endl;
	output_buffer = "";
}

socket_wrapper server;

bool connect() {
	server = socket_wrapper();
	if (!server.is_valid()) return false;
	log("Init server's socket: " + int_to_string(server.get_fd()));
	
	server.set_buffer();
	log("Init server's buffer");
	
	if (!server.connect("127.0.0.1", 8082)) return false;
	log("Server was connected to 127.0.0.1::8082");
	
	if (!server.make_nonblock()) return false;
	log("Server was made nonblock");
	return true;
}

bool try_to_reconnect() {
	return false;
}


int main() {
	init_log();
	//l.tie(cerr);

	if (!connect()) {
		log("Server unavailable");
		return 0;
	}

	epoll_wrapper epoll;
	log("Init epoll");

	epoll.add(server, EPOLLET | EPOLLIN);
	log("Server's socket was added to epoll with writing only");

	socket_wrapper input(stdin->_fileno);
	epoll.add(input, EPOLLET | EPOLLIN);
	log("Stdin was added to epoll");
	log("");

	for (; ; ) {
		vector <epoll_event> events = epoll.wait();
		bool exit_flag = false;
		for (epoll_event & cur : events) {
			if (cur.data.fd == stdin->_fileno) {
				log("Stdin ready for reading");
				string message;
				input.read(message);
				//cerr << message << endl;
				if (server.write_to_buffer(message) == socket_wrapper::SOCKET_CLOSED) {
					log("Server's buffer was overflowed");
					if (!try_to_reconnect()) {
						log("Server is unavailable!");
						exit_flag = true;
						break;
					}
					else {
						server.write_to_buffer(message);
					}
				}
				else {
					epoll.modify(server, EPOLLET | EPOLLIN | EPOLLOUT);
				}
			} 
			else {
				if (cur.events & EPOLLOUT) {
					log("Server ready for writing");
					server.write();	
					if (server.empty()) {
						epoll.modify(server, EPOLLET | EPOLLIN);
					}
					log("");
				}
				if (cur.events & EPOLLIN) {
					log("Server ready for reading");
					string message;
					int ans = server.read(message);
					if (ans == socket_wrapper::SOCKET_CLOSED) {	
						log("Server disconnects");				
						if (!try_to_reconnect()) {
							log("Server is unavailable!");
							exit_flag = true;
							break;
						}
					} else {
						for (char c : message) {
							if (c == '\n') flush_buffer();
							else output_buffer += c;
						}
					}
				}
			}
		}
		log("");
		if (exit_flag) {
			break;
		}
	}
	input.invalidate();
	return 0;
}