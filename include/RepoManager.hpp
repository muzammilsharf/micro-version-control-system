#pragma once

#include "Types.hpp"
#include "RepoTrie.hpp"
#include "AuthManager.hpp"
#include <unordered_map>
#include <vector>
#include <string>

/**
 * @class RepoManager
 * @brief Manages all repositories and coordinates repository CRUD operations.
 *
 * Owns the Global_Repo_Map (keyed by repository name) and the Repo_Trie
 * (for public repository indexing). Enforces permission checks and maintains
 * consistency between the map, Trie, and AuthManager's user repo lists.
 */
class RepoManager {
private:
    /**
     * @brief Map of repository name to Repository struct.
     * 
     * Keyed by repository name for O(1) average-time lookups.
     * All repositories in the system are stored here, both public and private.
     */
    std::unordered_map<std::string, Repository> global_repo_map;

    /**
     * @brief Prefix Tree indexing all public repositories by name.
     * 
     * Only public repositories are inserted into this Trie.
     * Enables O(L + K) autocomplete search on public repository names.
     */
    RepoTrie repo_trie;

    /**
     * @brief Auto-incrementing counter for generating unique repository IDs.
     * 
     * Incremented each time a new repository is created.
     * Used to construct IDs: "repo_" + counter + "_" + timestamp.
     */
    int repo_id_counter = 0;

public:
    /**
     * @brief Constructor: initializes empty repository map and Trie.
     */
    RepoManager();

    /**
     * @brief Create a new repository with validation and registration.
     * 
     * Validation:
     *   - Repository name must not be empty.
     *   - Repository name must not exceed 64 characters.
     *   - Repository name must be unique per owner. If the owner already owns
     *     a repository with this name: throw std::runtime_error("Repository name already exists.")
     *   - (Note: Different users can own repositories with the same name)
     * 
     * Behavior:
     *   - Generates unique ID: "repo_" + to_string(++repo_id_counter) + "_" + timestamp.
     *   - Creates Repository struct with id, name, owner_username, is_public.
     *   - Inserts into global_repo_map.
     *   - Calls auth.add_repo_to_user(owner_username, name).
     *   - If is_public: inserts name into repo_trie.
     * 
     * @param name Repository name (must be unique per owner, max 64 chars).
     * @param is_public true for public (indexed in Trie), false for private.
     * @param owner_username Username of the repository owner.
     * @param auth Reference to AuthManager for user repo list management.
     * @throws std::invalid_argument if name is empty or exceeds 64 characters.
     * @throws std::runtime_error if name already exists for this owner.
     */
    void create_repo(const std::string& name,
                     bool is_public,
                     const std::string& owner_username,
                     AuthManager& auth);

    /**
     * @brief Delete a repository with permission enforcement.
     * 
     * Permission Checks:
     *   - Lookup repo by name. If not found: throw std::runtime_error("Repository not found.")
     *   - If repo is private AND caller is not ADMIN AND caller is not owner:
     *     throw std::runtime_error("Repository not found.") — same error to prevent enumeration.
     *   - If repo is public OR caller is owner OR caller is ADMIN: proceed with deletion.
     * 
     * Behavior:
     *   - If is_public: calls repo_trie.remove(name).
     *   - Calls auth.remove_repo_from_user(owner_username, name).
     *   - Erases from global_repo_map.
     * 
     * @param name Name of repository to delete.
     * @param caller Session of the user performing the deletion.
     * @param auth Reference to AuthManager for user repo list cleanup.
     * @throws std::runtime_error if repository not found or access denied.
     */
    void delete_repo(const std::string& name,
                     const Session& caller,
                     AuthManager& auth);

    /**
     * @brief Retrieve a mutable reference to a repository with permission checks.
     * 
     * Permission Checks:
     *   - Lookup repo by name. If not found: throw std::runtime_error("Repository not found.")
     *   - If repo is private AND caller is not ADMIN AND caller is not owner:
     *     throw std::runtime_error("Repository not found.") — prevents enumeration.
     *   - Otherwise: return mutable reference.
     * 
     * @param name Name of repository to retrieve.
     * @param caller Session of the user requesting access.
     * @return Mutable reference to the Repository struct.
     * @throws std::runtime_error if repository not found or access denied.
     */
    Repository& get_repo(const std::string& name, const Session& caller);

    /**
     * @brief List all repositories owned by a specific user.
     * 
     * Iterates through global_repo_map and collects the names of all
     * repositories where owner_username matches.
     * 
     * @param username The owner's username.
     * @return Vector of repository names owned by username.
     */
    std::vector<std::string> list_user_repos(const std::string& username) const;

    /**
     * @brief List all public repositories in the system.
     * 
     * Iterates through global_repo_map and collects the names of all
     * repositories where is_public == true.
     * 
     * @return Vector of all public repository names.
     */
    std::vector<std::string> list_public_repos() const;

    /**
     * @brief Search for public repositories by prefix.
     * 
     * Delegates to repo_trie.search(prefix) after validating the prefix.
     * 
     * @param prefix The prefix to search for (must not be empty).
     * @return Vector of public repository names that start with prefix.
     *         Returns empty vector if no matches found.
     * @throws std::invalid_argument if prefix is empty.
     */
    std::vector<std::string> search_repos(const std::string& prefix) const;

    /**
     * @brief Load repositories from persistence (e.g., on startup).
     * 
     * Replaces the entire global_repo_map with the loaded map.
     * Rebuilds the repo_trie from scratch, inserting all public repositories.
     * Recalculates repo_id_counter based on the maximum ID found.
     * 
     * @param loaded_map Pre-constructed repository map to load (moved).
     */
    void load_repos(std::unordered_map<std::string, Repository>&& loaded_map);

    /**
     * @brief Retrieve a const reference to all repositories.
     * 
     * Used by PersistenceManager to serialize all repositories to disk.
     * 
     * @return Const reference to global_repo_map.
     */
    const std::unordered_map<std::string, Repository>& get_all_repos() const;
};
