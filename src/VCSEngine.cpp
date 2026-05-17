#include "VCSEngine.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <algorithm>

using namespace std;

// Static member initialization
int VCSEngine::commit_counter = 0;

// ============================================================================
// Private Helper Methods
// ============================================================================

string VCSEngine::generate_commit_hash() {
    // Get current time in milliseconds
    auto now = chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = chrono::duration_cast<chrono::milliseconds>(duration).count();
    
    // Increment counter and generate hash
    ++commit_counter;
    return to_string(millis) + "_" + to_string(commit_counter);
}

// Helper: count newlines in a string
static int count_lines(const string& content) {
    if (content.empty()) {
        return 0;
    }
    int count = 1;  // At least 1 line if content is not empty
    for (char c : content) {
        if (c == '\n') {
            ++count;
        }
    }
    return count;
}

// Helper: split string by newline
static vector<string> split_lines(const string& content) {
    vector<string> lines;
    if (content.empty()) {
        return lines;
    }
    
    stringstream ss(content);
    string line;
    while (getline(ss, line)) {
        lines.push_back(line);
    }
    
    return lines;
}

// ============================================================================
// Public Methods: File Editing
// ============================================================================

void VCSEngine::add_line(Repository& repo,
                         const string& filename,
                         int line_number,
                         const string& text,
                         const Session& caller) {
    if (caller.is_detached) {
        throw runtime_error("Detached HEAD: read-only state.");
    }

    auto cmd = make_unique<AddLineCommand>(repo.working_directory, filename, line_number, text);
    cmd->execute();
    repo.undo_stack.push(move(cmd));

    // Clear redo_stack (unique_ptr destructor handles cleanup)
    while (!repo.redo_stack.empty()) {
        repo.redo_stack.pop();
    }
}

void VCSEngine::delete_line(Repository& repo,
                            const string& filename,
                            int line_number,
                            const Session& caller) {
    if (caller.is_detached) {
        throw runtime_error("Detached HEAD: read-only state.");
    }

    auto cmd = make_unique<DeleteLineCommand>(repo.working_directory, filename, line_number);
    cmd->execute();
    repo.undo_stack.push(move(cmd));

    // Clear redo_stack
    while (!repo.redo_stack.empty()) {
        repo.redo_stack.pop();
    }
}

void VCSEngine::import_file(Repository& repo,
                            const string& file_path,
                            const Session& caller) {
    if (caller.is_detached) {
        throw runtime_error("Detached HEAD: read-only state.");
    }

    // Path validation: reject ".." and absolute paths
    if (file_path.find("..") != std::string::npos) {
        throw invalid_argument("Error: Invalid file path.");
    }

    // Check for absolute paths (starting with / or drive letter on Windows)
    if (!file_path.empty() && (file_path[0] == '/' || file_path[0] == '\\')) {
        throw invalid_argument("Error: Invalid file path.");
    }

    // On Windows, also reject drive letter paths (e.g., "C:")
    if (file_path.length() >= 2 && file_path[1] == ':') {
        throw invalid_argument("Error: Invalid file path.");
    }

    // Check file size
    try {
        uintmax_t file_size = filesystem::file_size(file_path);
        if (file_size > 1048576) {  // 1 MB
            throw runtime_error("File exceeds maximum size limit.");
        }
    } catch (const filesystem::filesystem_error&) {
        throw runtime_error("Error: Cannot read file.");
    }

    // Read file content
    ifstream file(file_path);
    if (!file.is_open()) {
        throw runtime_error("Error: Cannot read file.");
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string content = buffer.str();
    file.close();

    // Extract filename from path
    string filename = filesystem::path(file_path).filename().string();

    // Create and execute command
    auto cmd = make_unique<ImportFileCommand>(repo.working_directory, filename, content);
    cmd->execute();
    repo.undo_stack.push(std::move(cmd));

    // Clear redo_stack
    while (!repo.redo_stack.empty()) {
        repo.redo_stack.pop();
    }
}

void VCSEngine::remove_file(Repository& repo,
                            const string& filename,
                            const Session& caller) {
    if (caller.is_detached) {
        throw runtime_error("Detached HEAD: read-only state.");
    }

    if (repo.working_directory.find(filename) == repo.working_directory.end()) {
        throw runtime_error("File not found.");
    }

    auto cmd = make_unique<RemoveFileCommand>(repo.working_directory, filename);
    cmd->execute();
    repo.undo_stack.push(move(cmd));

    // Clear redo_stack
    while (!repo.redo_stack.empty()) {
        repo.redo_stack.pop();
    }
}

// ============================================================================
// Public Methods: Undo/Redo
// ============================================================================

void VCSEngine::undo(Repository& repo) {
    if (repo.undo_stack.empty()) {
        throw runtime_error("Nothing to undo.");
    }

    auto cmd = move(repo.undo_stack.top());
    repo.undo_stack.pop();
    cmd->undo();
    repo.redo_stack.push(move(cmd));
}

void VCSEngine::redo(Repository& repo) {
    if (repo.redo_stack.empty()) {
        throw runtime_error("Nothing to redo.");
    }

    auto cmd = move(repo.redo_stack.top());
    repo.redo_stack.pop();
    cmd->execute();
    repo.undo_stack.push(move(cmd));
}

// ============================================================================
// Public Methods: Commit and Checkout
// ============================================================================

void VCSEngine::commit(Repository& repo,
                       const string& message,
                       const Session& caller) {
    if (caller.is_detached) {
        throw runtime_error("Detached HEAD: read-only state.");
    }

    if (repo.working_directory.empty()) {
        throw runtime_error("Error: Working directory is empty.");
    }

    // Generate hash
    string hash = generate_commit_hash();

    // Get current timestamp
    auto now = chrono::system_clock::now();
    auto time_t_now = chrono::system_clock::to_time_t(now);
    stringstream ss;
    ss << put_time(localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    string timestamp = ss.str();

    // Create CommitNode
    auto node = make_unique<CommitNode>();
    node->hash = hash;
    node->message = message;
    node->timestamp = timestamp;
    node->parent_hash = repo.head_commit_hash;  // empty string if first commit
    node->file_snapshot = repo.working_directory;  // deep copy via assignment

    // Insert into commit_history
    repo.commit_history[hash] = move(node);

    // Update HEAD
    repo.head_commit_hash = hash;

    // Clear both stacks
    while (!repo.undo_stack.empty()) {
        repo.undo_stack.pop();
    }
    while (!repo.redo_stack.empty()) {
        repo.redo_stack.pop();
    }
}

void VCSEngine::checkout(Repository& repo,
                         const string& commit_hash_or_HEAD,
                         Session& caller) {
    // Handle "HEAD" special case
    if (commit_hash_or_HEAD == "HEAD") {
        caller.is_detached = false;
        if (!repo.head_commit_hash.empty()) {
            auto head_it = repo.commit_history.find(repo.head_commit_hash);
            if (head_it != repo.commit_history.end()) {
                repo.working_directory = head_it->second->file_snapshot;
            }
        }
        return;
    }

    // Dirty state check (unless zero commits)
    if (!repo.head_commit_hash.empty()) {
        auto head_it = repo.commit_history.find(repo.head_commit_hash);
        if (head_it != repo.commit_history.end()) {
            const auto& head_snapshot = head_it->second->file_snapshot;
            
            // Compare: if sizes differ or content differs
            if (repo.working_directory.size() != head_snapshot.size()) {
                throw runtime_error("Working directory not clean. Please commit or undo changes before checking out.");
            }
            
            for (const auto& pair : repo.working_directory) {
                auto snapshot_it = head_snapshot.find(pair.first);
                if (snapshot_it == head_snapshot.end() || snapshot_it->second != pair.second) {
                    throw runtime_error("Working directory not clean. Please commit or undo changes before checking out.");
                }
            }
        }
    }

    // Lookup commit hash
    auto it = repo.commit_history.find(commit_hash_or_HEAD);
    if (it == repo.commit_history.end()) {
        throw runtime_error("Commit not found.");
    }

    // Restore working_directory and set detached
    repo.working_directory = it->second->file_snapshot;
    caller.is_detached = true;
}

void VCSEngine::reset(Repository& repo, const Session& caller) {
    if (caller.is_detached) {
        throw runtime_error("Cannot reset in detached state.");
    }

    // Clear both stacks
    while (!repo.undo_stack.empty()) {
        repo.undo_stack.pop();
    }
    while (!repo.redo_stack.empty()) {
        repo.redo_stack.pop();
    }

    // Handle zero commits case
    if (repo.head_commit_hash.empty()) {
        repo.working_directory.clear();
        return;
    }

    // Otherwise restore from HEAD
    auto head_it = repo.commit_history.find(repo.head_commit_hash);
    if (head_it != repo.commit_history.end()) {
        repo.working_directory = head_it->second->file_snapshot;
    }
}

// ============================================================================
// Public Methods: Logging and Status
// ============================================================================

vector<string> VCSEngine::get_log(const Repository& repo, const Session& caller) const {
    // Permission check for private repos
    if (!repo.is_public && caller.role != Role::ADMIN && caller.username != repo.owner_username) {
        throw runtime_error("Repository not found.");
    }

    vector<string> log_entries;

    // Traverse from HEAD backward via parent_hash
    string current_hash = repo.head_commit_hash;
    while (!current_hash.empty()) {
        auto it = repo.commit_history.find(current_hash);
        if (it == repo.commit_history.end()) {
            break;  // Safety check: hash not found
        }

        const auto& node = it->second;
        string entry = "Hash: " + node->hash + " | Message: " + node->message + 
                           " | Time: " + node->timestamp;
        log_entries.push_back(entry);

        current_hash = node->parent_hash;
    }

    return log_entries;
}

string VCSEngine::get_status(const Repository& repo) const {
    if (repo.working_directory.empty()) {
        return "Working directory is empty.";
    }

    stringstream ss;
    for (const auto& pair : repo.working_directory) {
        int line_count = count_lines(pair.second);
        ss << "  " << pair.first << ": " << line_count << " lines\n";
    }

    string result = ss.str();
    // Remove trailing newline if present
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    return result;
}

string VCSEngine::view_file(const Repository& repo, const string& filename) const {
    auto it = repo.working_directory.find(filename);
    if (it == repo.working_directory.end()) {
        throw runtime_error("File not found in staging area.");
    }

    vector<string> lines = split_lines(it->second);
    
    stringstream ss;
    for (size_t i = 0; i < lines.size(); ++i) {
        ss << (i + 1) << ": " << lines[i] << "\n";
    }

    string result = ss.str();
    // Remove trailing newline if present
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    return result;
}
