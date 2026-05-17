#include "AuthManager.hpp"
#include <stdexcept>
#include <algorithm>

using namespace std;

// ============================================================================
// Private Helper Methods
// ============================================================================

string AuthManager::hash_to_string(const string& password) const {
    // djb2 hash algorithm
    size_t hash = 5381;
    for (char c : password) {
        hash = ((hash << 5) + hash) + static_cast<unsigned char>(c);
    }
    return to_string(hash);
}

// ============================================================================
// Constructor
// ============================================================================

AuthManager::AuthManager() : user_id_counter(0) {
    // users_map is default-initialized (empty)
}

// ============================================================================
// Public Methods
// ============================================================================

void AuthManager::register_user(const string& username,
                                const string& password,
                                Role role,
                                const Session& caller_session) {
    // Validate inputs
    if (username.empty()) {
        throw invalid_argument("Username cannot be empty.");
    }
    if (password.empty()) {
        throw invalid_argument("Password cannot be empty.");
    }

    // RBAC Enforcement: only ADMIN can assign roles other than USER
    if (caller_session.is_active && role != Role::USER) {
        if (caller_session.role != Role::ADMIN) {
            throw runtime_error("Unauthorized: only ADMIN can assign roles.");
        }
    }

    // Check for duplicate username
    if (users_map.find(username) != users_map.end()) {
        throw runtime_error("Username already exists.");
    }

    // Hash the password
    string hashed_pwd = hash_to_string(password);

    // Create new User
    User new_user;
    new_user.id = to_string(user_id_counter);
    new_user.username = username;
    new_user.hashed_password = hashed_pwd;
    new_user.role = role;
    // repo_list is default-initialized (empty vector)

    // Insert into map
    users_map[username] = new_user;

    // Increment counter
    ++user_id_counter;
}

Session AuthManager::login(const string& username, const string& password) {
    // Special case: guest login
    if (username == "guest") {
        return Session{"guest", Role::GUEST, true, false, ""};
    }

    // Lookup username in users_map
    auto it = users_map.find(username);
    if (it == users_map.end()) {
        throw runtime_error("Authentication failed.");
    }

    const User& user = it->second;

    // Hash input password and compare
    string input_hash = hash_to_string(password);
    if (input_hash != user.hashed_password) {
        throw runtime_error("Authentication failed.");
    }

    // Success: return active session with user's role
    return Session{username, user.role, true, false, ""};
}

User& AuthManager::get_user(const string& username) {
    auto it = users_map.find(username);
    if (it == users_map.end()) {
        throw runtime_error("User not found.");
    }
    return it->second;
}

const unordered_map<string, User>& AuthManager::get_all_users() const {
    return users_map;
}

void AuthManager::add_repo_to_user(const string& username, const string& repo_name) {
    auto it = users_map.find(username);
    if (it == users_map.end()) {
        throw runtime_error("User not found.");
    }

    it->second.repo_list.push_back(repo_name);
}

void AuthManager::remove_repo_from_user(const string& username, const string& repo_name) {
    auto it = users_map.find(username);
    if (it == users_map.end()) {
        throw runtime_error("User not found.");
    }

    auto& repo_list = it->second.repo_list;
    
    // Use std::find and erase to remove the repo_name
    auto repo_it = find(repo_list.begin(), repo_list.end(), repo_name);
    if (repo_it != repo_list.end()) {
        repo_list.erase(repo_it);
    }
    // No-op if not found (no exception)
}

void AuthManager::load_users(const unordered_map<string, User>& loaded_map) {
    users_map = loaded_map;
    
    // Update user_id_counter based on loaded users
    // Counter should be set to one more than the maximum ID
    user_id_counter = 0;
    for (const auto& pair : users_map) {
        const User& user = pair.second;
        try {
            int id = stoi(user.id);
            if (id >= user_id_counter) {
                user_id_counter = id + 1;
            }
        } catch (...) {
            // If ID is not a valid integer, skip it
        }
    }
}
