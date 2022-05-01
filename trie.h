#ifndef clox_trie_h
#define clox_trie_h

#include <stdbool.h>

typedef struct {
    bool marked;
    int value;
    Trie* nodes[26];
} Trie;

void Trie_Init(Trie* trie);
void Trie_Add(Trie* trie, char* key, int value);
int Trie_Get(char* key);

#endif