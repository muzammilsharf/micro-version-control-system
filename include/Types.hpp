#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <stack>
#include <memory>
#include "Command.hpp"

enum class Role { ADMIN, USER, GUEST };

struct CommitNode {
    std::string hash;
    std::string message;
    std::string timestamp;
    std::string parent_hash;  // "" means root commit (no parent)
    std::unordered_map<std::string, std::string> file_snapshot;  // filename -> full text content
};

struct Repository {
    std::string id;  // unique UUID or timestamp-counter string
    std::string name;
    std::string owner_username;
    bool is_public;
    std::unordered_map<std::string, std::string> working_directory;  // filename -> content
    std::unordered_map<std::string, std::unique_ptr<CommitNode>> commit_history;  // hash -> node
    std::string head_commit_hash;  // "" if no commits exist yet
    std::stack<std::unique_ptr<Command>> undo_stack;
    std::stack<std::unique_ptr<Command>> redo_stack;

    // Repository is NOT copyable because unique_ptr stacks cannot be copied
    Repository() = default;
    Repository(Repository&&) = default;
    Repository& operator=(Repository&&) = default;
    Repository(const Repository&) = delete;
    Repository& operator=(const Repository&) = delete;
};

struct User {
    std::string id;
    std::string username;
    std::string hashed_password;
    Role role;
    std::vector<std::string> repo_list;  // stores REPO NAMES (not IDs) for direct map lookup
};

struct Session {
    std::string username;
    Role role;
    bool is_active = false;
    bool is_detached = false;  // true when in detached HEAD state after checkout
    std::string active_repo_name;   // name of the repo the user is currently working in
};

// Free functions for role conversion
std::string role_to_string(Role r);
Role string_to_role(const std::string& s);
