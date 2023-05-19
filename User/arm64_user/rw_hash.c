#include "rw_hash.h"
#include "define.h"

hash_table* create_hash_table() {
    hash_table* table = (hash_table*)malloc(sizeof(hash_table));
    memset(table->buckets, 0, sizeof(table->buckets));
    int i;
    for (i = 0; i < HASH_SIZE; i++) {
        pthread_rwlock_init(&table->lock[i], NULL);
    }
    return table;
}

unsigned int hash(const char* key) {
    unsigned int hashval = 0;
    while (*key) {
        hashval = hashval * 31 + (*key++);
    }
    return hashval % HASH_SIZE;
}

void* hash_table_get(hash_table* table, const char* key) {
    unsigned int hashval = hash(key);
    pthread_rwlock_rdlock(&table->lock[hashval]);
    hash_node* node = table->buckets[hashval];
    while (node != NULL) {
        if (strcmp(node->key, key) == 0) {
            pthread_rwlock_unlock(&table->lock[hashval]);
            return node->value;
        }
        node = node->next;
    }
    pthread_rwlock_unlock(&table->lock[hashval]);
    return NULL;
}

void hash_table_put(hash_table* table, const char* key, void* value) {
    unsigned int hashval = hash(key);
    pthread_rwlock_wrlock(&table->lock[hashval]);
    hash_node* node = table->buckets[hashval];
    while (node != NULL) {
        if (strcmp(node->key, key) == 0) {
            node->value = value;
            pthread_rwlock_unlock(&table->lock[hashval]);
            return;
        }
        node = node->next;
    }
    node = (hash_node*)malloc(sizeof(hash_node));
    strcpy(node->key, key);
    node->value = value;
    node->next = table->buckets[hashval];
    table->buckets[hashval] = node;
    pthread_rwlock_unlock(&table->lock[hashval]);
}

void hash_table_remove(hash_table* table, const char* key) {
    unsigned int hashval = hash(key);
    pthread_rwlock_wrlock(&table->lock[hashval]);
    hash_node* node = table->buckets[hashval];
    hash_node* prev = NULL;
    while (node != NULL) {
        if (strcmp(node->key, key) == 0) {
            if (prev == NULL) {
                table->buckets[hashval] = node->next;
            } else {
                prev->next = node->next;
            }
            free(node);
            pthread_rwlock_unlock(&table->lock[hashval]);
            return;
        }
        prev = node;
        node = node->next;
    }
    pthread_rwlock_unlock(&table->lock[hashval]);
}

void destroy_hash_table(hash_table* table) {
    int i;
    for (i = 0; i < HASH_SIZE; i++) {
        pthread_rwlock_destroy(&table->lock[i]);
        hash_node* node = table->buckets[i];
        while (node != NULL) {
            hash_node* next = node->next;
            free(node);
            node = next;
        }
    }
    free(table);
}

void save_hash_table(hash_table* table, const char* filename) {
    FILE* fp = fopen(filename, "wb");
    if (fp == NULL) {
        printf("Error: Failed to open file %s for writing.\n", filename);
        return;
    }
    int i;
    for (i = 0; i < HASH_SIZE; i++) {
        pthread_rwlock_rdlock(&table->lock[i]);
        hash_node* node = table->buckets[i];
        while (node != NULL) {
            fwrite(node->key, sizeof(char), MD5_STR_LEN+1, fp);
            fwrite(node->value, sizeof(struct FileInfo_table_server), 1, fp);
            node = node->next;
        }
        pthread_rwlock_unlock(&table->lock[i]);
    }
    fclose(fp);
}

hash_table* load_hash_table(const char* filename) {
    FILE* fp = fopen(filename, "a+");
    if (fp == NULL) {
        printf("Error: Failed to open file %s for reading.\n", filename);
        return NULL;
    }
    hash_table* table = create_hash_table();
    char key[MD5_STR_LEN+1];
    while (fread(key, sizeof(char), MD5_STR_LEN+1, fp)) {
        struct FileInfo_table_server *value = (struct FileInfo_table_server *)malloc(sizeof(struct FileInfo_table_server));
        fread(value, sizeof(struct FileInfo_table_server), 1, fp);
        hash_table_put(table, key, value);
    }
    fclose(fp);
    return table;
}