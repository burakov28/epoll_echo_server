#pragma once

#include <iostream>
#include <algorithm>
#include <fstream>
#include <map>

std::string int_to_string(int a) {
	if (a == 0) return "0";
	std::string ret;

	while (a > 0) {
		ret += '0' + a % 10;
		a /= 10;
	}
	std::reverse(ret.begin(), ret.end());
	return ret;
}



void erase_file(const char *file) {
	std::filebuf fb;
	fb.open(file, std::ios::out);
	std::ostream output(&fb);
	output << "";
	fb.close();
}

void print_to_file(const char *file, std::string message) {
	std::filebuf fb;
	fb.open(file, std::ios::app);
	std::ostream output(&fb);
	output << message << std::endl;
	fb.close();
}

std::string delete_empty_symbols(std::string str) {
	std::string ret;
	for (char c : str) {
		if (c == '\n') {
			ret += "\\n";
		}
		else ret += c;
	}
	return ret;
}
