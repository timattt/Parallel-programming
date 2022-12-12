/*
 * AtomicMakableReference.h
 *
 *  Created on: Dec 12, 2022
 *      Author: timat
 */

#ifndef CORE_ATOMICMARKABLEREFERENCE_H_
#define CORE_ATOMICMARKABLEREFERENCE_H_

#include <atomic>

class SkipNode;
class MarkableReference;

class AtomicMarkableReference {
public:
	// fields
	std::atomic<MarkableReference*> ref;

	// constructor
	AtomicMarkableReference();
	AtomicMarkableReference(SkipNode *val, bool marked);
	~AtomicMarkableReference();

	// methods
	struct SkipNode* get_reference();
	struct SkipNode* get(bool &mark);
	void set(struct SkipNode *value, bool mark);
	void set_marked(bool mark);
	bool compare_and_swap(struct SkipNode *expected_value, bool expected_mark, struct SkipNode *new_value, bool new_mark);
};

#endif /* CORE_ATOMICMARKABLEREFERENCE_H_ */
