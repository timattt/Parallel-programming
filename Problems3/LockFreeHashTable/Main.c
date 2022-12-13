#include "Core/HashTable.h"

#include <stdio.h>
#include <stdlib.h>

#define SZ 1000

int main() {
	//=======================
	// RANDOMIZED TEST
	//=======================

	struct hashTable *ht = conc_hashtable_create(0);

	int bitmap[SZ] = { 0 };

	int ins = 0;
	int er = 0;

	for (int j = 0; j < 10000; j++) {
		int i = rand() % SZ;
		switch (rand() % 3) {
		case 0:
			ins++;
			conc_hashtable_insert(ht, i, i*i);
			bitmap[i] = 1;
			break;
		case 1:
			er++;
			conc_hashtable_delete(ht, i);
			bitmap[i] = 0;
			break;
		case 2:
			if ((conc_hashtable_find(ht, i) == i*i) != (bitmap[i] > 0)) {
				printf("err: %d\n", j);
				printf("total ins=%d, total erases=%d\n", ins, er);
				for (int k = 0; k < SZ; k++) {
					printf("%d, bitmap=%d, cont=%d\n", k, bitmap[k],
							conc_hashtable_find(ht, k) == k*k);
				}
				return 0;
			}
			break;
		}
	}

	ht = conc_hashtable_create(0);

	conc_hashtable_insert(ht, 8, 17);
	conc_hashtable_insert(ht, 2, 14);
	conc_hashtable_insert(ht, 9, 15);
	conc_hashtable_insert(ht, 13, 19);

	dump_hash_graphwiz(ht, 0);

	conc_hashtable_insert(ht, 10, 333);

	dump_hash_graphwiz(ht, 1);

	printf("total ins=%d, total erases=%d\n", ins, er);
	printf("OK\n");

	return 0;
}
