/*
 * AtomicMakableReference.cpp
 *
 *  Created on: Dec 12, 2022
 *      Author: timat
 */

#include "AtomicMarkableReference.h"
#include "MarkableReference.h"
#include "SkipNode.h"

AtomicMarkableReference::AtomicMarkableReference() {
	ref.store(new MarkableReference(nullptr, false));
}

AtomicMarkableReference::AtomicMarkableReference(struct SkipNode *val, bool marked) {
	ref.store(new MarkableReference(val, marked));
}

AtomicMarkableReference::~AtomicMarkableReference() {
	MarkableReference *temp = ref.load();
	delete temp;
}

struct SkipNode* AtomicMarkableReference::get_reference() {
	return ref.load()->val;
}

// Stores the value of this references marked flag in reference
struct SkipNode* AtomicMarkableReference::get(bool &mark) {
	MarkableReference *temp = ref.load();
	mark = temp->marked;
	return temp->val;
}

void AtomicMarkableReference::set(struct SkipNode *value, bool mark) {
	MarkableReference *curr = ref.load();
	if (value != curr->val || mark != curr->marked) {
		ref.store(new MarkableReference(value, mark));
	}
}

void AtomicMarkableReference::set_marked(bool mark) {
	MarkableReference *curr = ref.load();
	if (mark != curr->marked) {
		ref.store(new MarkableReference(curr->val, mark));
	}
}

// Atomically sets the value of both the reference and mark to the given
// update values if the current reference is equal to the expected reference
// and the current mark is equal to the expected mark. returns true on success
bool AtomicMarkableReference::compare_and_swap(struct SkipNode *expected_value, bool expected_mark, struct SkipNode *new_value,
		bool new_mark) {
	MarkableReference *curr = ref.load();
	return (expected_value == curr->val
			&& 	expected_mark == curr->marked
			&& ((new_value == curr->val && new_mark == curr->marked) || // if already equal, return true by shortcircuiting
					ref.compare_exchange_strong(curr, new MarkableReference(new_value, new_mark)))); // otherwise, attempt compare and swap
}
