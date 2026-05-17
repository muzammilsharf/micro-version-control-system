/* Generate the Types.hpp header file. This must contain the foundational data structures:
1. An enum class Role with ADMIN, USER, and GUEST.
2. A User struct containing: id (string), password_hash (string), role (Role enum), and repo_list (std::vector).
3. A Repository struct containing: id, owner_id, is_public (bool), head_commit_hash. It must also encapsulate a working_directory (std::unordered_map<string, string>), a commit_history (std::unordered_map<string, CommitNode>), and two stacks (undo_stack and redo_stack) of std::unique_ptr<Command>.
4. A CommitNode struct containing: hash, message, timestamp, parent_hash (strings), and file_snapshot (std::unordered_map<string, string>).
Use standard include guards or #pragma once. */

#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

enum class Role {
    ADMIN,
    USER,
    GUEST
};