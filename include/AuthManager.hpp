#pragma once

#include "Types.hpp"
#include <unordered_map>
#include <string>

/**
 * @class AuthManager
 * @brief Manages user authentication, registration, and RBAC enforcement.
 *
 * Owns the Users_HashMap and provides O(1) average-time lookup for user queries.
 * Handles password hashing via the djb2 algorithm and RBAC role validation.
 * All user registration and login operations pass through this manager.
 */
class AuthManager {
private:
    /**
     * @brief Map of username to User struct.
     * 
     * Keyed by username for O(1) average-time authentication lookups.
     * Updated by register_user, add_repo_to_user, and remove_repo_from_user.
     */
    std::unordered_map<std::string, User> users_map;

    /**
     * @brief Auto-incrementing counter for generating unique user IDs.
     * 
     * Incremented each time a new user is registered.
     * IDs are assigned sequentially as strings (e.g., "0", "1", "2").
     */
    int user_id_counter = 0;

    /**
     * @brief Hashes a password using the djb2 algorithm and returns the hash as a string.
     * 
     * djb2 algorithm:
     *   hash = 5381
     *   for each character c: hash = ((hash << 5) + hash) + c
     * 
     * @param password The plaintext password to hash.
     * @return String representation of the djb2 hash value.
     */
    std::string hash_to_string(const std::string& password) const;

public:
    /**
     * @brief Constructor: initializes an empty users_map and resets the ID counter.
     */
    AuthManager();

    /**
     * @brief Register a new user with a given username, password, and role.
     * 
     * RBAC Enforcement:
     *   - If caller_session.is_active and role != Role::USER, caller must be ADMIN.
     *     Else: throw std::runtime_error("Unauthorized: only ADMIN can assign roles.")
     *   - If caller is inactive (GUEST), they can only register as USER role.
     * 
     * Username Uniqueness:
     *   - If username already exists: throw std::runtime_error("Username already exists.")
     * 
     * Behavior:
     *   - Hashes the password via hash_to_string.
     *   - Creates a new User struct with auto-incremented ID.
     *   - Inserts into users_map.
     *   - Increments user_id_counter.
     * 
     * @param username New username (must be unique).
     * @param password Plaintext password (will be hashed).
     * @param role Role to assign (ADMIN, USER, or GUEST).
     * @param caller_session The session of the user performing registration.
     * @throws std::runtime_error if unauthorized or username exists.
     * @throws std::invalid_argument if username or password is empty.
     */
    void register_user(const std::string& username,
                       const std::string& password,
                       Role role,
                       const Session& caller_session);

    /**
     * @brief Authenticate a user by username and password.
     * 
     * Special Case - Guest Login:
     *   - If username == "guest": return Session{"guest", Role::GUEST, true, false, ""}.
     * 
     * Normal Login:
     *   - Lookup username in users_map.
     *   - If not found: throw std::runtime_error("Authentication failed.")
     *   - Hash input password and compare against stored hash.
     *   - If mismatch: throw std::runtime_error("Authentication failed.")
     *   - On success: return Session{username, user.role, true, false, ""}.
     * 
     * @param username Username to authenticate.
     * @param password Password to verify.
     * @return Session object with is_active=true and role set accordingly.
     * @throws std::runtime_error if username not found or password mismatch.
     */
    Session login(const std::string& username, const std::string& password);

    /**
     * @brief Retrieve a reference to a User by username.
     * 
     * @param username The username to look up.
     * @return Reference to the User struct.
     * @throws std::runtime_error("User not found.") if username does not exist.
     */
    User& get_user(const std::string& username);

    /**
     * @brief Retrieve a const reference to all registered users.
     * 
     * Used by the list_users command to iterate over all users.
     * 
     * @return Const reference to users_map.
     */
    const std::unordered_map<std::string, User>& get_all_users() const;

    /**
     * @brief Add a repository name to a user's repo_list vector.
     * 
     * Appends repo_name to the user's vector for repository ownership tracking.
     * 
     * @param username The username of the repository owner.
     * @param repo_name The name of the repository to add.
     * @throws std::runtime_error if username does not exist.
     */
    void add_repo_to_user(const std::string& username, const std::string& repo_name);

    /**
     * @brief Remove a repository name from a user's repo_list vector.
     * 
     * Erases repo_name from the user's vector. This is a no-op if repo_name
     * is not in the vector (no exception thrown).
     * 
     * @param username The username whose repo_list is updated.
     * @param repo_name The name of the repository to remove.
     * @throws std::runtime_error if username does not exist.
     */
    void remove_repo_from_user(const std::string& username, const std::string& repo_name);

    /**
     * @brief Replace the entire users_map with a loaded state from persistence.
     * 
     * Called by PersistenceManager::load_all_state() on startup to restore
     * user data from disk. Completely replaces the current users_map.
     * 
     * @param loaded_map The pre-constructed users_map to load.
     */
    void load_users(const std::unordered_map<std::string, User>& loaded_map);
};
