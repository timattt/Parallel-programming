/*
 * SkipNode.cpp
 *
 *  Created on: Dec 12, 2022
 *      Author: timat
 */

#include "SkipNode.h"

#include "AtomicMarkableReference.h"
#include "MarkableReference.h"
#include "SkipNode.h"

SkipNode::SkipNode(int k, int v, int forward_size) : key(k), value(v), top_level(forward_size) {
	intialize_forward(forward_size, nullptr);
}

// Constructor for sentinel nodes head and NIL
SkipNode::SkipNode(int k, int forward_size, SkipNode *forward_target) :
		key(k), top_level(forward_size) {
	intialize_forward(forward_size, forward_target);
}

SkipNode::~SkipNode() {
	forward.clear(); // calls destructors on AtomicMarkableReferences
}

void SkipNode::intialize_forward(const int forward_size, struct SkipNode *forward_target) {
	forward = std::vector<AtomicMarkableReference>(forward_size);
	for (auto i = 0; i != forward_size; ++i) {
		forward[i].set(forward_target, false);
	}
}
