#include "HashTable.h"

struct node * mk_node(struct node *n, long long bit) {
	return (struct node *) (((long long) n) | bit);
}

struct node* get_node(struct node * n) {
	return (struct node*) (((long long) n) & ~(long long) 0x1);
}

struct node * get_bit(struct node * n) {
	return (struct node *)((long long) n & 0x1);
}

void delete_node(struct node * node) {
	assert(get_bit(node) == 0);
	free (get_node(node));
}
