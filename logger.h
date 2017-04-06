#pragma once

#include "utils.h"
#include <iostream>

struct logger {
	const char *log_file;
	std::ostream *stream_ptr;

	logger() : log_file(nullptr), stream_ptr(nullptr) {}

	logger(const char *_log_file) : log_file(_log_file), stream_ptr(nullptr) {
		erase_file(log_file);
	}

	void log(std::string message) {
		message = delete_empty_symbols(message);
		print_to_file(log_file, message);
		if (stream_ptr != nullptr) {
			std::ostream & out = *stream_ptr;
			out << "LOG MESSAGE: " << message << std::endl;
		}
	}

	void tie(std::ostream & stream) {
		stream_ptr = &stream;
	}

	void untie() {
		stream_ptr = nullptr;
	}
};