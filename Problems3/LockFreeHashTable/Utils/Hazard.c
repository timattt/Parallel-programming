#include "../Core/HashTable.h"

pthread_key_t hazard_pointer;

void** get_hazard_table() {
	void** tb = pthread_getspecific(hazard_pointer);
	if (!tb) {
		tb = calloc(sizeof(void*), 3);
		pthread_setspecific(hazard_pointer, tb);
	}
	return tb;
}

void* get_hazardous_pointer(void** pp, int hazard_index) {
	void* res = *pp;
	get_hazard_table()[hazard_index] = res;
	return res;
}

/*Clean the bottom 2 bits*/
void** unmask(void** p) {
	return (void**) ((uintptr_t) p & ~(uintptr_t) 0x3);
}

/*
 Use this version of get_hazardous_pointer if either
 @pp or the value it points to might be masked.
 */
void* get_hazardous_pointer_with_mask(void** pp, int hazard_index) {
	void* res = *unmask(pp);
	get_hazard_table()[hazard_index] = unmask(res);
	return res;
}

void clear_hazardous_pointer(int hazard_index) {
	get_hazard_table()[hazard_index] = NULL;
}

void set_hazardous_pointer(void* pp, int hazard_index) {
	get_hazard_table()[hazard_index] = pp;
}

void set_hazardous_pointer_with_mask(void* pp, int hazard_index) {
	get_hazard_table()[hazard_index] = unmask(pp);
}

void clear_hazardous_pointers() {
	void* *tb = get_hazard_table();
	tb[0] = NULL;
	tb[1] = NULL;
	tb[2] = NULL;
}

void*get_current_hazardous_pointer (int hazard_index) {
	return get_hazard_table () [hazard_index];
}
