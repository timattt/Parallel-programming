#include "../Core/HashTable.h"

unsigned reverse_value(unsigned k) {
	int i;
	unsigned r = 0;
	for (i = 0; i < 32; ++i) {
		unsigned bit = (k & (1 << i)) >> i;
		r |= bit << (31 - i);
	}
	return r;
}

unsigned hash_key(MAP_TYPE key) {
	return (unsigned) ((uintptr_t) (key) * 2654435761u);
}

unsigned hash_regular_key(unsigned k) {
	return reverse_value(k | 0x80000000);
}

unsigned hash_dummy_key(unsigned k) {
	return reverse_value(k & ~0x80000000);
}

unsigned get_parent(unsigned b) {
	int i;
	for (i = 31; i >= 0; --i) {
		if (b & (1 << i))
			return b & ~(1 << i);
	}
	return 0;
}
