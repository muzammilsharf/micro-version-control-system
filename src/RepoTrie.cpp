#include "RepoTrie.hpp"
#include <stdexcept>

using namespace std;
// ============================================================================
// Private Helper Methods
// ============================================================================

void RepoTrie::destroy(TrieNode* node) {
    if (node == nullptr) {
        return;
    }

    // Post-order traversal: first destroy all children
    for (auto& pair : node->children) {
        destroy(pair.second);
    }

    // Then delete the node itself
    delete node;
}

void RepoTrie::dfs_collect(TrieNode* node, vector<string>& results) const {
    if (node == nullptr) {
        return;
    }

    // If this node marks the end of a word, collect its repo name
    if (node->is_end_of_word && !node->repo_name.empty()) {
        results.push_back(node->repo_name);
    }

    // Recursively visit all children
    for (auto& pair : node->children) {
        dfs_collect(pair.second, results);
    }
}

// ============================================================================
// Constructor & Destructor
// ============================================================================

RepoTrie::RepoTrie() {
    root = new TrieNode();
}

RepoTrie::~RepoTrie() {
    // Recursively destroy all children of root
    for (auto& pair : root->children) {
        destroy(pair.second);
    }

    // Delete the root node itself
    delete root;
    root = nullptr;
}

// ============================================================================
// Insert Method
// ============================================================================

void RepoTrie::insert(const string& repo_name) {
    if (repo_name.empty()) {
        throw std::invalid_argument("Repository name cannot be empty.");
    }

    TrieNode* current = root;

    // Traverse or create a path for each character
    for (char c : repo_name) {
        // If child for this character doesn't exist, create it
        if (current->children.find(c) == current->children.end()) {
            current->children[c] = new TrieNode();
        }

        // Move to the child node
        current = current->children[c];
    }

    // At terminal node, mark as end of word and store repo name
    current->is_end_of_word = true;
    current->repo_name = repo_name;
}

// ============================================================================
// Remove Method (Tombstone Approach)
// ============================================================================

bool RepoTrie::remove(const string& repo_name) {
    if (repo_name.empty()) {
        return false;
    }

    TrieNode* current = root;

    // Traverse to the terminal node
    for (char c : repo_name) {
        if (current->children.find(c) == current->children.end()) {
            // Path doesn't exist, repo_name was never inserted
            return false;
        }

        current = current->children[c];
    }

    // Check if this node represents the end of the repo_name
    if (!current->is_end_of_word || current->repo_name != repo_name) {
        // The repo_name was not actually inserted at this node
        return false;
    }

    // Tombstone approach: mark as not-end-of-word and clear repo_name
    // But DO NOT delete the node or its structural path
    current->is_end_of_word = false;
    current->repo_name = "";

    return true;
}

// ============================================================================
// Search Method
// ============================================================================

vector<string> RepoTrie::search(const string& prefix) const {
    vector<string> results;

    if (prefix.empty()) {
        throw invalid_argument("Search prefix cannot be empty.");
    }

    TrieNode* current = root;

    // Traverse to the end of the prefix
    for (char c : prefix) {
        if (current->children.find(c) == current->children.end()) {
            // Prefix path doesn't exist, return empty results
            return results;
        }

        current = current->children[c];
    }

    // From the terminal node of the prefix, DFS-collect all repo names
    dfs_collect(current, results);

    return results;
}

// ============================================================================
// Clear Method
// ============================================================================

void RepoTrie::clear() {
    // Recursively destroy all children of the current root
    for (auto& pair : root->children) {
        destroy(pair.second);
    }

    // Clear the root's children map
    root->children.clear();

    // Reset root's metadata
    root->is_end_of_word = false;
    root->repo_name = "";
}
