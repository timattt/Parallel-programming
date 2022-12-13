#include "HashTable.h"

struct node * list_find_hp(struct hashTable *ht, unsigned bucket, MAP_TYPE key, unsigned hash_code, struct node *** out_prev) {
	struct node ** table = NULL;
	struct node * cur = NULL;
	struct node * next = NULL;
	struct node ** prev = NULL;

try_again:

	table = get_hazardous_pointer((void**) &ht->table, 0);
	struct node ** head = &table[bucket];
	prev = head;
	cur = get_hazardous_pointer((void**) prev, 1);

	while (1) {
		if (get_node(cur) == NULL) {
			goto done;
		}

		next = get_hazardous_pointer_with_mask((void**) &cur->next, 0);

		unsigned cur_hash = cur->hash_code;
		MAP_TYPE cur_key = cur->key;

		if (atomic_load (prev) != mk_node(get_node(cur), 0)) {
			goto try_again;
		}

		if (!get_bit(next)) {
			if (cur_hash > hash_code || (cur_hash == hash_code && cur_key == key)) {
				goto done;
			}

			prev = &get_node(cur)->next;
			set_hazardous_pointer_with_mask(cur, 2);
		} else {
			if (atomic_compare_and_swap(prev, mk_node(get_node(cur), 0), mk_node(get_node(next), 0))) {
				delete_node(get_node(cur));
			} else {
				goto try_again;
			}
		}
		cur = next;
		set_hazardous_pointer_with_mask(next, 1);
	}

done:

	*out_prev = prev;
	return cur;
}


struct node * list_insert_hp(struct hashTable *ht, unsigned bucket, struct node *node) {
	struct node * res = NULL;
	struct node ** prev = NULL;
	MAP_TYPE key = node->key;
	unsigned hash_code = node->hash_code;

	store_barrier();

	while (1) {
		res = list_find_hp(ht, bucket, key, hash_code, &prev);
		if (res && res->hash_code == node->hash_code && res->key == node->key) {
			return res;
		}
		node->next = mk_node(get_node(res), 0);
		set_hazardous_pointer(node, 0);
		if (atomic_compare_and_swap(prev, mk_node(get_node(res), 0), mk_node(node, 0)))
			return node;
	}
}

struct node* list_delete_hp (hashTable *ht, unsigned bucket, MAP_TYPE key, MAP_TYPE hash_code) {
	struct node* res = NULL;
	struct node** prev = NULL;
	struct node* next = NULL;
	while (1) {
		res = list_find_hp (ht, bucket, key, hash_code, &prev);
		if (!res || res->hash_code != hash_code || res->key != key)
			return NULL;

		next = get_current_hazardous_pointer (0);
		if (!atomic_compare_and_swap (&get_node (res)->next, mk_node (get_node (next), 0), mk_node (get_node (next), 1))) {
			continue;
		}
		if (atomic_compare_and_swap (prev, mk_node (get_node (res), 0), mk_node (get_node (next), 0))) {
			delete_node (get_node (res));
		}

		return res;
	}
}
