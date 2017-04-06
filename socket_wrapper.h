#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <cassert>


struct socket_wrapper {	
	static const int READ_BUFFER_SIZE = 1024;
	static const size_t BUFFER_SIZE = 256 * 1024;
	static const int SOCKET_CLOSED = -1; 

	int *socket_ptr, *counter_ptr;
	char **buffer;
	size_t *buffer_pos, buffer_size;

	socket_wrapper() : socket_ptr(new int(::socket(AF_INET, SOCK_STREAM, 0))), counter_ptr(new int(1)), buffer(nullptr),
						buffer_pos(new size_t(0)), buffer_size(0) {}

	socket_wrapper(int fd) : socket_ptr(new int(fd)), counter_ptr(new int(1)), buffer(nullptr), buffer_pos(new size_t(0)), buffer_size(0) {}

	socket_wrapper(socket_wrapper const & other) : socket_ptr(other.socket_ptr), counter_ptr(other.counter_ptr), buffer(other.buffer),
						buffer_pos(other.buffer_pos), buffer_size(other.buffer_size) {
		++(*counter_ptr);
	}

	~socket_wrapper() {
		close();
	}

	socket_wrapper & operator=(socket_wrapper const & other) {
		close();
		socket_ptr = other.socket_ptr;
		counter_ptr = other.counter_ptr;
		++(*counter_ptr);
		buffer = other.buffer;
		buffer_pos = other.buffer_pos;
		buffer_size = other.buffer_size;
		return *this;
	}

	void close() {

		if (!is_valid()) return;
		--(*counter_ptr);
		if (*counter_ptr == 0) destroy();
	}

	void destroy() {		
		if (is_valid()) {
			::close(*socket_ptr);
		}
		if (buffer != nullptr) {
			delete *buffer;
			delete buffer;
		}
		delete socket_ptr;
		socket_ptr = nullptr;
		delete counter_ptr;
		delete buffer_pos;
	}


	int get_fd() const {
		assert(is_valid());
		return *socket_ptr;
	} 

	void set_buffer(size_t size = BUFFER_SIZE) {
		if (buffer != nullptr) {
			delete *buffer;
		}
		buffer = new char*();
		*buffer = new char[size];
		buffer_size = size;
		*buffer_pos = 0;
	}

	bool is_valid() const {
		return (socket_ptr != nullptr) && (*socket_ptr != -1);
	}
	
	void bind(const char * host, unsigned long port) {
		assert(is_valid());
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));

		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		inet_aton(host, &addr.sin_addr);

		::bind(*socket_ptr, (sockaddr *) &addr, sizeof(addr));
	}

	void listen(int count = 128) {
		assert(is_valid());
		::listen(*socket_ptr, count);
	}

	bool connect(const char * host, unsigned long port) {
		assert(is_valid());
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		
		hostent *server = gethostbyname(host);
		memcpy(&addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
		if (::connect(*socket_ptr, (sockaddr *) &addr, sizeof(addr)) < 0) {
			return false;
		}
		return true;
	}

	bool make_nonblock() {
		assert(is_valid());
		if (fcntl(*socket_ptr, F_SETFL, fcntl(*socket_ptr, F_GETFL, 0) | O_NONBLOCK) < 0) {
			return false;
		}
		return true;
	}

	socket_wrapper accept() {
		assert(is_valid());
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		socklen_t len = 0;
		return socket_wrapper(::accept(*socket_ptr, (sockaddr *) &addr, &len));
	}

	int read(std::string & message) { //read to message and close if end of stream was reached
		assert(is_valid());
		char read_buffer[READ_BUFFER_SIZE];
		memset(read_buffer, 0, sizeof(read_buffer));

		message = "";
		int was_read;
		int ans = 0;
		if ((was_read = ::read(*socket_ptr, read_buffer, READ_BUFFER_SIZE - 1)) > 0) {
			message += std::string(read_buffer);
			ans = was_read;
		}

		if (was_read == 0) {
			return SOCKET_CLOSED;
		}

		return ans;
	}

	size_t rest_of_buffer() const {
		assert(is_valid());
		return buffer_size - *buffer_pos;
	}

	int write_to_buffer(std::string message) { //close socket in case of overflowing;
		assert(is_valid());
		if (rest_of_buffer() < message.size()) {
			return SOCKET_CLOSED;
		}

		for (int i = 0; i < (int) message.size(); ++i) {
			(*(*buffer + *buffer_pos)) = message[i];
			++(*buffer_pos);
		}
		return 0;
	}

	bool empty() const {
		assert(is_valid());
		return *buffer_pos == buffer_size;
	}

	void write() {	
		assert(is_valid());
		if (empty()) return;
		int was_wrote = ::write(*socket_ptr, *buffer, *buffer_pos);
		int has = *buffer_pos;
		if (was_wrote > 0) {
			for (int i = was_wrote; i < has; ++i) {
				(*buffer)[i - was_wrote] = (*buffer)[i];
			}
			*buffer_pos -= was_wrote;
		}
	}

	void invalidate() {
		socket_ptr = nullptr;
	}
};

bool operator==(socket_wrapper const & s1, socket_wrapper const & s2) {	
	return s1.get_fd() == s2.get_fd();
}

bool operator<(socket_wrapper const & s1, socket_wrapper const & s2) {
	return s1.get_fd() < s2.get_fd();
}