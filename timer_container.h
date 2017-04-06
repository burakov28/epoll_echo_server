#pragma once

#include <list>
#include <map>
#include <ctime>


template <typename T>
struct timer_container {
	typedef typename std::list<std::pair<unsigned long long, T>> List;

	std::multimap<unsigned long long, List*> heads;
	std::map<unsigned long long, typename std::multimap<unsigned long long, List*>::iterator> timeouts;
	std::map<List*, unsigned long long> list_timeout;

	~timer_container() {
		for (auto p : heads) {
			delete p.second;
		}
	}

	struct iterator {
		typename List::iterator list_it;
		typename std::map<unsigned long long, typename std::multimap<unsigned long long, List*>::iterator>::iterator timeout_it;

		iterator() {}

		iterator(typename List::iterator const & _list_it, typename std::map<unsigned long long, typename std::multimap<unsigned long long, List*>::iterator>::iterator const & _timeout_it) {
			list_it = _list_it;
			timeout_it = _timeout_it;
		}

		iterator(iterator const & other) : list_it(other.list_it), timeout_it(other.timeout_it) {}

		iterator& operator=(iterator const & other) {
			list_it = other.list_it;
			timeout_it = other.timeout_it;
			return *this;
		}

		T& get_element() const {
			return (*list_it).second;
		}

		unsigned long long get_expiration() const {
			return (*list_it).first;
		}

		unsigned long long get_timeout() const {
			return (*timeout_it).first;
		}

		List* get_list_ptr() const {
			return (*((*timeout_it).second)).second;
		}

		typename std::multimap<unsigned long long, List*>::iterator get_head_it() const {
			return timeout_it->second;
		}
	};

	iterator add(T const & element, unsigned long long timeout, unsigned long long expiration = -1) {
		if (expiration == (unsigned long long) -1) {
			expiration = time(NULL) + timeout;
		}

		List *list;
		if (timeouts.count(timeout) == 0) {
			list = new List();
			list_timeout[list] = timeout;
			timeouts[timeout] = heads.insert({expiration, list});
		}
		else list = timeouts[timeout]->second;	

		auto list_it = list->insert(list->end(), {expiration, element});
		return iterator(list_it, timeouts.find(timeout));
	}

	iterator get_next() {
		List *list = heads.begin()->second;
		return iterator(list->begin(), timeouts.find(list_timeout[list]));
	}

	unsigned long long get_next_expiration() {
		return get_next().get_expiration();
	}

	void remove(iterator const & it) {
		List *list = it.get_list_ptr();
		if (list->size() == 1) {
			list_timeout.erase(list);
			heads.erase(it.get_head_it());
			timeouts.erase(it.timeout_it);
			delete list;
			return;
		}

		if (it.list_it == list->begin()) {
			heads.erase(it.get_head_it());
			list->erase(it.list_it);
			heads.insert({list->front().first, list});
		} else {
			list->erase(it.list_it); 
		}
	}

	iterator modify(iterator const & it, unsigned long long new_expiration = -1) {
		T element = it.get_element();
		unsigned long long timeout = it.get_timeout();
		if (new_expiration == (unsigned long long) -1) {
			new_expiration = time(NULL) + timeout;
		}
		remove(it);
		return add(element, timeout, new_expiration);
	}
};