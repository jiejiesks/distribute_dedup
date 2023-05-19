#include <stdio.h>
#include <stdlib.h>
#include "rw_hash.h"


int main() {
    hash_table* table = create_hash_table();

    // Insert some nodes into the hash table
    person *p1 = malloc(sizeof(person));
    p1->id = 1;
    strcpy(p1->name, "Alice");
    hash_table_put(table,"123",p1);

    person *p2 = malloc(sizeof(person));
    p2->id = 2;
    strcpy(p2->name, "Bob");
    hash_table_put(table,"124",p2);

    person *p4 = hash_table_get(table, "124");
    if (p4 != NULL) {
        printf("Person with ID 124: %s (%d)\n", p4->name, p4->id);
    } else {
        printf("Person with ID 124 not found\n");
    }
    // Save the hash table to a file
    save_hash_table(table, "table.dat");

    // Load the hash table from the file
    hash_table *table2 = load_hash_table("table.dat");

    // Get a node from the hash table
    person *p3 = (person *)hash_table_get(table2, "123");
    if (p3 != NULL) {
        printf("Person with ID 123: %s (%d)\n", p3->name, p3->id);
    } else {
        printf("Person with ID 123 not found\n");
    }

    person *p5 = (person *)hash_table_get(table2, "124");
    if (p5 != NULL) {
        printf("Person with ID 123: %s (%d)\n", p5->name, p5->id);
    } else {
        printf("Person with ID 123 not found\n");
    }

    // Delete a node from the hash table
    hash_table_remove(table2, "124");

    // Free the memory used by the hash tables
    destroy_hash_table(table);
    destroy_hash_table(table2);

    return 0;
}
