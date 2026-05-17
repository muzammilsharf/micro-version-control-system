#pragma once

#include <string>
#include <vector>
#include <unordered_map>

/**
 * @struct TrieNode
 * @brief A node in the Prefix Tree (Trie) for repository name indexing.
 *
 * Each node has children indexed by character and metadata to mark word boundaries.
 * Uses raw owning pointers; memory is managed by RepoTrie's destructor.
 */
struct TrieNode {
    /**
     * @brief Map of character to child TrieNode pointers (raw owning pointers).
     * 
     * Child nodes are allocated dynamically and freed by RepoTrie's destroy() method.
     */
    std::unordered_map<char, TrieNode*> children;

    /**
     * @brief Indicates if this node is the end of a valid repository name.
     * 
     * Default: false. Set to true when a complete repo name ends at this node.
     * Used to distinguish between intermediate nodes and word boundaries.
     */
    bool is_end_of_word = false;

    /**
     * @brief Stores the full repository name at terminal nodes.
     * 
     * Only meaningful when is_end_of_word == true.
     * Default: empty string. Enables O(K) result collection during prefix search.
     */
    std::string repo_name = "";
};

/**
 * @class RepoTrie
 * @brief Custom Prefix Tree (Trie) for efficient repository name indexing and search.
 *
 * Supports O(L) insertion and deletion (where L is the name length),
 * and O(L + K) prefix search (where K is the number of matching results).
 *
 * Memory is managed via explicit destructor. The tombstone deletion strategy
 * preserves structural nodes to avoid invalidating shared prefixes.
 */
class RepoTrie {
private:
    /**
     * @brief Pointer to the root TrieNode.
     * 
     * The root node itself is not deleted; only its descendants are freed
     * in the destructor via the destroy() helper method.
     */
    TrieNode* root;

    /**
     * @brief Recursively frees all TrieNode descendants.
     * 
     * Traverses the entire tree in post-order (children first, then node itself),
     * deallocating each node via delete. Must be called on root->children in destructor.
     *
     * @param node The node whose subtree is to be freed. Can be nullptr.
     */
    void destroy(TrieNode* node);

    /**
     * @brief Helper for DFS-based prefix search.
     * 
     * Recursively collects all repository names reachable from the given node
     * where is_end_of_word == true.
     *
     * @param node Starting node for DFS traversal.
     * @param results Vector to append matching repo names to.
     */
    void dfs_collect(TrieNode* node, std::vector<std::string>& results) const;

public:
    /**
     * @brief Constructor: allocates and initializes the root TrieNode.
     */
    RepoTrie();

    /**
     * @brief Destructor: recursively frees all TrieNode descendants.
     * 
     * Ensures complete deallocation to satisfy NFR-REL-02 (no memory leaks).
     */
    ~RepoTrie();

    /**
     * @brief Insert a repository name into the Trie.
     * 
     * Traverses or creates a path for each character in repo_name.
     * At the terminal node, sets is_end_of_word = true and repo_name = the full name.
     * 
     * @param repo_name The repository name to insert.
     * @throws std::invalid_argument if repo_name is empty.
     */
    void insert(const std::string& repo_name);

    /**
     * @brief Remove a repository name from the Trie (tombstone approach).
     * 
     * Traverses to the terminal node of repo_name.
     * If found, sets is_end_of_word = false and repo_name = "".
     * Does NOT delete structural nodes; shared prefixes remain intact.
     * 
     * @param repo_name The repository name to remove.
     * @return true if the name was found and marked; false if not found.
     */
    bool remove(const std::string& repo_name);

    /**
     * @brief Search for all repository names with a given prefix.
     * 
     * Traverses the Trie to the end of the prefix.
     * If the prefix path exists, performs DFS from that node to collect
     * all repository names where is_end_of_word == true.
     * 
     * @param prefix The prefix to search for.
     * @return Vector of repository names starting with prefix.
     *         Returns empty vector if prefix not found or no matches.
     * @throws std::invalid_argument if prefix is empty.
     */
    std::vector<std::string> search(const std::string& prefix) const;

    /**
     * @brief Clear all repository entries from the Trie.
     * 
     * Frees all nodes (except root, which is reallocated) and resets the tree.
     * This provides a complete reset of the Trie to initial state.
     */
    void clear();
};
