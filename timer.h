#pragma once

#include <list>
#include <map>
#include <ctime>


template <typename T>
struct timer_container {
	struct iterator {
		typename std::list<std::pair<unsigned long long, T*>>::iterator list_it;
		unsigned long long timeout;

		iterator(typename std::list<std::pair<unsigned long long, T*>>::iterator _it, unsigned long long _timeout) {
			list_it = _it;
			timeout = _timeout;
		}
	};

	std::map<unsigned long long, typename std::list<std::pair<unsigned long long, T*>> *> timeouts; 

	iterator add(T* element, unsigned long long timeout, unsigned long long expiration = time(NULL)) {
		if (timeouts.find(timeout) == timeouts.end()) {
			timeouts[timeout] = new typename std::list<std::pair<unsigned long long, T*>>();
		}
		timeouts[timeout]->push_back({{expiration, element}});
	}

	unsigned long long get_next() {
		return (*(timeouts.begin())).first;
	}

	void remove_next() {
		(*(timeouts.begin())).second->pop_front();
		if ((*(timeouts.begin())).second->empty()) {
			timeouts.erase(timeouts.begin());
		}
	}

	void remove(iterator it) {
		timeouts[it.timeout]->erase(it.list_it);
		if (timeouts[it.timeout]->empty()) {
			timeouts.erase(it.timeout);	
		}
	}


	void modify(iterator it, unsigned long long new_expiration) {
		T* element = (*(it.list_it)).second;
		remove(it);
		add(element, it.timeout, new_expiration);
	}
};