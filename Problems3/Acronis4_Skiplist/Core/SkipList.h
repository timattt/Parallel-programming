/*
 * SkipList.h
 *
 *  Created on: Dec 12, 2022
 *      Author: timat
 */

#ifndef CORE_SKIPLIST_H_
#define CORE_SKIPLIST_H_

#include <cstdint>
#include <iostream>

class SkipNode;
class RCUManager;

#define MAX_LEVELS 16
#define PROBABILITY 0.5

class SkipList {
public:

	// constructors
	SkipList();
	~SkipList();

public:

	// public methods
	bool find_with_gc(int search_key, SkipNode **preds, SkipNode **succs);
	void insert(int key, int val);
	SkipNode* remove(int key);
	void print(std::ostream &os);
	void dump_graph(SkipNode ** a, SkipNode ** b);
	uint32_t size();
	bool conts(SkipNode* nd);

private:

	// private methods
	int random_level() const;

public:

	// fields
	SkipNode *head;
	SkipNode *NIL;
};

#endif /* CORE_SKIPLIST_H_ */
