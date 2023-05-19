#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define HASH_SIZE 1000

typedef struct _hash_node {
    char key[33];
    void* value;
    struct _hash_node* next;
} hash_node;

typedef struct _hash_table {
    hash_node* buckets[HASH_SIZE];
    pthread_rwlock_t lock[HASH_SIZE];
} hash_table;

typedef struct person{
    int id;
    char name[50];
} person;

hash_table* create_hash_table();
void hash_table_put(hash_table* , const char* , void* );
void* hash_table_get(hash_table* , const char* );
void hash_table_remove(hash_table* , const char* );
void destroy_hash_table(hash_table* );
void save_hash_table(hash_table* , const char* );
hash_table* load_hash_table(const char* filname);