/*
 * MarkableReference.h
 *
 *  Created on: Dec 12, 2022
 *      Author: timat
 */

#ifndef CORE_MARKABLEREFERENCE_H_
#define CORE_MARKABLEREFERENCE_H_

class MarkableReference {
public:
	// fields
	struct SkipNode *val = nullptr;
	bool marked = false;

	// constructor
	MarkableReference(MarkableReference &other) : val(other.val), marked(other.marked) {}
	MarkableReference(struct SkipNode *value, bool mark) : val(value), marked(mark) {}
};

#endif /* CORE_MARKABLEREFERENCE_H_ */
