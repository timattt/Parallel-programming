/*
 * SkipNode.h
 *
 *  Created on: Dec 12, 2022
 *      Author: timat
 */

#ifndef CORE_SKIPNODE_H_
#define CORE_SKIPNODE_H_

#include <vector>

class AtomicMarkableReference;

class SkipNode {
public:
	// fields
	int key;
	int value;
	int top_level;
	std::vector<AtomicMarkableReference> forward;// Vector of atomic, markable (logical delete) forward nodes

	// constructor
	SkipNode(int k, int v, int forward_size);
	SkipNode(int k, int forward_size, SkipNode *forward_target);
	~SkipNode();

	// methods
	void intialize_forward(int forward_size, struct SkipNode *forward_target);
};

#endif /* CORE_SKIPNODE_H_ */
