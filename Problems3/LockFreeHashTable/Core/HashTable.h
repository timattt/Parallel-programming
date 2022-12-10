#pragma once

// INCLUDES
//============================================
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include "../Utils/Atomics.h"
//============================================


// CONSTANTS
//============================================
#define LOAD_FACTOR 0.75f
#define MAP_TYPE long long
//============================================


// STRUCTURES
//============================================
typedef struct node {
	struct node * next;
	unsigned hash_code;
	MAP_TYPE key;
	MAP_TYPE value;
} node;

typedef struct hashTable {
	struct node ** table;
	unsigned count, size;
	int lock_value;
} hashTable;
//============================================


// GLOBAL METHODS
//============================================
// hazard
void** get_hazard_table();
void* get_hazardous_pointer(void ** pp, int hazard_index);
void** unmask(void ** p);
void* get_hazardous_pointer_with_mask(void ** pp, int hazard_index);
void clear_hazardous_pointer(int hazard_index);
void set_hazardous_pointer(void * pp, int hazard_index);
void set_hazardous_pointer_with_mask(void* pp, int hazard_index);
void clear_hazardous_pointers();
void*get_current_hazardous_pointer(int hazard_index);

// hash table
void initialize_bucket(struct hashTable * ht, struct node ** table, unsigned bucket);
void resize_table(struct hashTable * ht, unsigned size);
int conc_hashtable_insert(struct hashTable *ht, MAP_TYPE key, MAP_TYPE value);
MAP_TYPE conc_hashtable_find(struct hashTable *ht, MAP_TYPE key);
struct hashTable* conc_hashtable_create(int lock_value);
MAP_TYPE conc_hashtable_delete (struct hashTable *ht, MAP_TYPE key);
void dump_hash(struct hashTable *ht);
void dump_hash_graphwiz(struct hashTable *ht, int id);

// list
struct node * list_find_hp(struct hashTable *ht, unsigned bucket, MAP_TYPE key, unsigned hash_code, struct node *** out_prev);
struct node * list_insert_hp(struct hashTable *ht, unsigned bucket, struct node *node);
struct node* list_delete_hp (hashTable *ht, unsigned bucket, MAP_TYPE key, MAP_TYPE hash_code);

// node
struct node * mk_node(struct node *n, long long bit);
struct node* get_node(struct node * n);
struct node * get_bit(struct node * n);
void delete_node(struct node * node);

// hash
unsigned reverse_value(unsigned k);
unsigned hash_key(MAP_TYPE key);
unsigned hash_regular_key(unsigned k);
unsigned hash_dummy_key(unsigned k);
unsigned get_parent(unsigned b);
//============================================


// GLOBAL FIELDS
//============================================
pthread_key_t hazard_pointer;
//============================================
