#include "HashTable.h"

void initialize_bucket(struct hashTable *ht, struct node ** table, unsigned bucket) {
	struct node * res = NULL;
	unsigned parent = get_parent(bucket);

	if (atomic_load (&table [parent]) == NULL) {
		initialize_bucket(ht, table, parent);
	}

	struct node *node = calloc(sizeof(struct node), 1);
	node->key = (MAP_TYPE) bucket;
	node->hash_code = hash_dummy_key(bucket);

	res = list_insert_hp(ht, parent, node);
	if (get_node(res) != node) {
		free(node);
		node = get_node(res);
	}

	table = get_hazardous_pointer((void**) &ht->table, 1);
	atomic_store(&table[bucket], mk_node(node, 0));
}

void resize_table(struct hashTable *ht, unsigned size) {
	struct node **old_table = get_hazardous_pointer((void**) &ht->table, 0);
	struct node **new_table = calloc(sizeof(struct node*), size * 2);
	memcpy(new_table, old_table, sizeof(struct node*) * size);
	if (!atomic_compare_and_swap(&ht->size, size, size * 2)) {
		free(new_table);
		return;
	}
	if (!atomic_compare_and_swap((void** )&ht->table, old_table, new_table)) {
		free(new_table);
	}
}

int conc_hashtable_insert(struct hashTable *ht, MAP_TYPE key, MAP_TYPE value) {
	unsigned hash = hash_key(key);
	struct node *node = calloc(sizeof(struct node), 1);
	struct node * *table = get_hazardous_pointer((void**) &ht->table, 0);

	node->hash_code = hash_regular_key(hash);
	node->key = key;
	node->value = value;

	unsigned bucket = hash % ht->size;
	if (table[bucket] == NULL)
		initialize_bucket(ht, table, bucket);

	if (get_node(list_insert_hp(ht, bucket, node)) != node) {
		free(node);
		clear_hazardous_pointers();
		return 0;
	}

	float size = (float) ht->size;
	if (atomic_fetch_and_inc (&ht->count) / size > LOAD_FACTOR) {
		resize_table(ht, size);
	}

	return 1;
}

MAP_TYPE conc_hashtable_find(struct hashTable *ht, MAP_TYPE key) {
	struct node * res = NULL;
	struct node ** prev = NULL;
	unsigned hash = hash_key(key);
	unsigned bucket = hash % ht->size;
	struct node * res_node = NULL;
	struct node ** table = get_hazardous_pointer((void**) &ht->table, 0);

	if (table[bucket] == NULL) {
		initialize_bucket(ht, table, bucket);
	}

	hash = hash_regular_key(hash);
	res = list_find_hp(ht, bucket, key, hash, &prev);
	res_node = get_node(res);
	if (res_node && res_node->hash_code == hash && res_node->key == key) {
		MAP_TYPE val = -1;
		if (ht->lock_value) {
			val = (MAP_TYPE) get_hazardous_pointer_with_mask((void**)&res_node->value, 0);
			clear_hazardous_pointer(1);
			clear_hazardous_pointer(2);
		} else {
			val = res_node->value;
			clear_hazardous_pointers();
		}
		return val;
	}
	return -1;
}

struct hashTable* conc_hashtable_create(int lock_value) {
	struct hashTable *res = calloc(sizeof(struct hashTable), 1);
	res->lock_value = lock_value;
	res->size = 2;
	res->table = calloc(sizeof(struct node), 2);
	res->table[0] = calloc(sizeof(struct node), 1);
	res->table[0]->hash_code = hash_dummy_key(0);
	res->table[0]->key = (MAP_TYPE) 0;
	return res;
}

MAP_TYPE conc_hashtable_delete(struct hashTable *ht, MAP_TYPE key) {
	struct node* res = NULL;
	unsigned hash = hash_key(key);
	unsigned bucket = hash % ht->size;
	struct node ** table = get_hazardous_pointer ((void**)&ht->table, 0);
	MAP_TYPE value = 0;

	if (table [bucket] == NULL) {
		initialize_bucket (ht, table, bucket);
	}

	hash = hash_regular_key (hash);
	res = list_delete_hp (ht, bucket, key, hash);

	if (!res) {
		return -1;
	}

	atomic_fetch_and_dec (&ht->count);

	if (ht->lock_value) {
		value = (MAP_TYPE)get_hazardous_pointer_with_mask ((void**)&get_node (res)->value, 0);
		clear_hazardous_pointer (1);
		clear_hazardous_pointer (2);
	} else {
		value = get_node (res)->value;
		clear_hazardous_pointers ();
	}

	atomic_store ((void**)&res->value, NULL);

	return value;
}

void dump_hash(struct hashTable *ht) {
	int i = 0;
	struct node *cur = get_node(ht->table[0]);
	printf("---------\n");
	for (i = 0; i < ht->size; ++i) {
		if (ht->table[i]) {
			printf("root [%d] -> %p\n", i, ht->table[i]);
		} else {
			printf("root [%d] -> empty\n", i);
		}
	}
	while (cur) {
		char buck = 0;
		int i = 0;
		for (; i < ht->size; i++) {
			if (ht->table[i] == cur) {
				buck = 1;
				break;
			}
		}

		printf("node %p hash %08x key %p deleted %d", cur, cur->hash_code, (void*) (uintptr_t) cur->key, (int) get_bit(cur->next));

		if (buck) {
			printf(" <- bucket %d\n", i);
		} else {
			printf("\n");
		}

		cur = get_node(cur->next);
	}
	printf("---------\n");
}

void dump_hash_graphwiz(struct hashTable *ht, int id) {
	remove("src.txt");
	FILE * ff = fopen("src.txt", "a+");
	fprintf(ff, "digraph mygraph {\n");


	fprintf(ff, "subgraph clusterbuckets {\n");
	fprintf(ff, "style=filled;\ncolor=lightgrey;\n");
	for (int i = 0; i < ht->size; ++i) {
		if (ht->table[i]) {
			fprintf(ff, "buc%d[label=\"%d\"];\n", i, i);
		} else {
			//fprintf(ff, "buc%d[label=\"empty\"]\n", i);
		}
	}

	fprintf(ff, "}\n");


	struct node *cur = get_node(ht->table[0]);
	fprintf(ff, "subgraph clusternodes {\n");
	int index = 0;
	while (cur) {
		char buck = 0;
		int i = 0;
		for (; i < ht->size; i++) {
			if (ht->table[i] == cur) {
				buck = 1;
				break;
			}
		}

		fprintf(ff, "vert%d[label=\"hash=%08x, key=%p\", shape=box", index, cur->hash_code, (void*) (uintptr_t) cur->key);

		if (buck) {
			fprintf(ff, ", color=red, style=filled");
		}else{
			//fprintf(ff, ", color=green");
		}

		fprintf(ff, "];\n");


		cur = get_node(cur->next);
		index++;
	}
	fprintf(ff, "}\n");



	// nodes
	index = 0;
	struct node * prev = NULL;
	cur = get_node(ht->table[0]);
	while (cur) {
		if (prev != NULL) {
			fprintf(ff, "vert%d -> vert%d;\n", index-1, index);
		}

		prev = cur;
		cur = get_node(cur->next);
		index++;
	}


	 // buckets
	cur = get_node(ht->table[0]);
	index = 0;
	while (cur) {
		int i = 0;
		for (; i < ht->size; i++) {
			if (ht->table[i] == cur) {
				break;
			}
		}

		if (i < ht->size)
			fprintf(ff, "buc%d -> vert%d;\n", i, index);

		cur = get_node(cur->next);
		index++;
	}



	fprintf(ff, "}\n");
	fclose(ff);
	char cmd[100] = {0};
	sprintf(cmd, "dot -Tpng src.txt -o src%d.png", id);
	system(cmd);
}

