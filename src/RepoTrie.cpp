/*mplement a custom Prefix Tree (Trie) named RepoTrie.
It must support O(L) time complexity for insertions and prefix matching, where L is the prefix length.
The terminal node (is_end_of_word == true) must store a repository_name payload.
Implement a tombstone deletion method: to delete, traverse to the terminal node, set is_end_of_word = false, and clear the payload, leaving structural nodes intact.*/