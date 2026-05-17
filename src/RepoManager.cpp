#include "RepoManager.hpp"
#include <stdexcept>
#include <chrono>
#include <sstream>

using namespace std;

// ============================================================================
// Constructor
// ============================================================================

RepoManager::RepoManager() : repo_id_counter(0) {
    // global_repo_map and repo_trie are default-initialized
}

// ============================================================================
// Public Methods
// ============================================================================

void RepoManager::create_repo(const string& name,
                               bool is_public,
                               const string& owner_username,
                               AuthManager& auth) {
    // Validation: name not empty
    if (name.empty()) {
        throw invalid_argument("Repository name cannot be empty.");
    }

    // Validation: name length <= 64 chars
    if (name.length() > 64) {
        throw invalid_argument("Repository name exceeds 64 characters.");
    }

    // Check: if repo exists with same name AND same owner, reject
    auto it = global_repo_map.find(name);
    if (it != global_repo_map.end() && it->second.owner_username == owner_username) {
        throw runtime_error("Repository name already exists.");
    }

    // Generate unique ID: "repo_" + counter + "_" + timestamp
    ++repo_id_counter;
    auto now = chrono::system_clock::now();
    auto timestamp = chrono::system_clock::to_time_t(now);
    std::string repo_id = "repo_" + std::to_string(repo_id_counter) + "_" + 
                          std::to_string(timestamp);

    // Create Repository struct
    Repository new_repo;
    new_repo.id = repo_id;
    new_repo.name = name;
    new_repo.owner_username = owner_username;
    new_repo.is_public = is_public;
    // working_directory, commit_history, undo_stack, redo_stack are default-initialized

    // Insert into global_repo_map
    global_repo_map[name] = move(new_repo);

    // Add repo to user's repo list
    auth.add_repo_to_user(owner_username, name);

    // If public, insert into Trie
    if (is_public) {
        repo_trie.insert(name);
    }
}

void RepoManager::delete_repo(const string& name,
                               const Session& caller,
                               AuthManager& auth) {
    // Lookup repo
    auto it = global_repo_map.find(name);
    if (it == global_repo_map.end()) {
        throw runtime_error("Repository not found.");
    }

    const Repository& repo = it->second;

    // Permission check: mask private repos unless caller is owner or ADMIN
    if (!repo.is_public && caller.role != Role::ADMIN && caller.username != repo.owner_username) {
        throw runtime_error("Repository not found.");
    }

    // If public, remove from Trie
    if (repo.is_public) {
        repo_trie.remove(name);
    }

    // Remove from user's repo list
    auth.remove_repo_from_user(repo.owner_username, name);

    // Erase from global_repo_map
    global_repo_map.erase(it);
}

Repository& RepoManager::get_repo(const string& name, const Session& caller) {
    // Lookup repo
    auto it = global_repo_map.find(name);
    if (it == global_repo_map.end()) {
        throw runtime_error("Repository not found.");
    }

    Repository& repo = it->second;

    // Permission check: mask private repos unless caller is owner or ADMIN
    if (!repo.is_public && caller.role != Role::ADMIN && caller.username != repo.owner_username) {
        throw runtime_error("Repository not found.");
    }

    return repo;
}

vector<string> RepoManager::list_user_repos(const string& username) const {
    vector<string> result;

    for (const auto& pair : global_repo_map) {
        if (pair.second.owner_username == username) {
            result.push_back(pair.first);  // pair.first is the repo name
        }
    }

    return result;
}

vector<string> RepoManager::list_public_repos() const {
    vector<string> result;

    for (const auto& pair : global_repo_map) {
        if (pair.second.is_public) {
            result.push_back(pair.first);  // pair.first is the repo name
        }
    }

    return result;
}

vector<string> RepoManager::search_repos(const string& prefix) const {
    if (prefix.empty()) {
        throw invalid_argument("Error: Please provide a search term.");
    }

    return repo_trie.search(prefix);
}

void RepoManager::load_repos(unordered_map<string, Repository>&& loaded_map) {
    // Replace global_repo_map
    global_repo_map = std::move(loaded_map);

    // Clear and rebuild Trie from all public repositories
    repo_trie.clear();
    for (const auto& pair : global_repo_map) {
        if (pair.second.is_public) {
            repo_trie.insert(pair.first);
        }
    }

    // Recalculate repo_id_counter from loaded repos
    repo_id_counter = 0;
    for (const auto& pair : global_repo_map) {
        const string& repo_id = pair.second.id;
        
        // Parse ID format: "repo_<counter>_<timestamp>"
        // Extract the counter portion
        size_t first_underscore = repo_id.find('_');
        size_t second_underscore = repo_id.find('_', first_underscore + 1);
        
        if (first_underscore != string::npos && second_underscore != string::npos) {
            try {
                string counter_str = repo_id.substr(first_underscore + 1, 
                                                         second_underscore - first_underscore - 1);
                int counter = stoi(counter_str);
                if (counter >= repo_id_counter) {
                    repo_id_counter = counter + 1;
                }
            } catch (...) {
                // If parsing fails, skip this ID
            }
        }
    }
}

const unordered_map<string, Repository>& RepoManager::get_all_repos() const {
    return global_repo_map;
}
