#include <stdlib.h>
#include <string.h>

#include "trie.h"

void Trie_Init(Trie* trie) {
    for (int i = 0; i < 26; i++) {
        trie->nodes[i] = NULL;
    }
}

void Trie_Add(Trie* trie, char* key, int value) {
    Trie* curr = trie;
    for (int i = 0; i < strlen(key); i++) {
        char ch = key[i];
        Trie* next = curr->nodes[ch - 'a'];
    }
}

int Trie_Get(Trie* trie, char* key) {

}

bool empty(Trie* trie) {
    return trie == NULL;
}