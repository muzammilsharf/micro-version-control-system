#pragma once

#include "Types.hpp"
#include "Command.hpp"
#include "RepoManager.hpp"
#include <vector>
#include <string>

/**
 * @class VCSEngine
 * @brief Handles all version control operations: file editing, committing, checkout, undo/redo, and history.
 *
 * Manages the complete lifecycle of repository state including working directory changes,
 * command stacks, commit history, and detached HEAD states.
 * All operations enforce detached HEAD constraints and validate state transitions.
 */
class VCSEngine {
private:
    /**
     * @brief Static counter for generating unique commit hashes.
     * 
     * Incremented with each commit to ensure hash uniqueness.
     * Combined with millisecond timestamp in generate_commit_hash().
     */
    static int commit_counter;

    /**
     * @brief Generate a unique commit hash.
     * 
     * Format: to_string(millisecond_timestamp) + "_" + to_string(++commit_counter)
     * Ensures deterministic, collision-resistant hashes for each commit.
     * 
     * @return Unique commit hash string.
     */
    std::string generate_commit_hash();

public:
    /**
     * @brief Add a line of text to a file in the working directory.
     * 
     * Detached HEAD Check:
     *   - If caller.is_detached: throw std::runtime_error("Detached HEAD: read-only state.")
     * 
     * Behavior:
     *   - Creates a unique_ptr<AddLineCommand> with repo.working_directory reference.
     *   - Calls execute() to insert the line.
     *   - Pushes command to repo.undo_stack.
     *   - Clears repo.redo_stack (unique_ptr destructor handles cleanup).
     * 
     * @param repo Repository to modify (working_directory updated).
     * @param filename File to edit.
     * @param line_number 1-indexed line number for insertion.
     * @param text Text to insert.
     * @param caller Session performing the operation.
     * @throws std::runtime_error if detached HEAD or file not found or line out of range.
     * @throws std::invalid_argument if line_number is invalid.
     */
    void add_line(Repository& repo,
                  const std::string& filename,
                  int line_number,
                  const std::string& text,
                  const Session& caller);

    /**
     * @brief Delete a line from a file in the working directory.
     * 
     * Detached HEAD Check:
     *   - If caller.is_detached: throw std::runtime_error("Detached HEAD: read-only state.")
     * 
     * Behavior:
     *   - Creates a unique_ptr<DeleteLineCommand> with repo.working_directory reference.
     *   - Calls execute() to remove the line (stores deleted content for undo).
     *   - Pushes command to repo.undo_stack.
     *   - Clears repo.redo_stack.
     * 
     * @param repo Repository to modify.
     * @param filename File to edit.
     * @param line_number 1-indexed line number to delete.
     * @param caller Session performing the operation.
     * @throws std::runtime_error if detached HEAD or file not found or line out of range.
     * @throws std::invalid_argument if line_number is invalid.
     */
    void delete_line(Repository& repo,
                     const std::string& filename,
                     int line_number,
                     const Session& caller);

    /**
     * @brief Import an external file into the working directory.
     * 
     * Detached HEAD Check:
     *   - If caller.is_detached: throw std::runtime_error("Detached HEAD: read-only state.")
     * 
     * Path Validation:
     *   - Rejects paths containing ".." (path traversal).
     *   - Rejects absolute paths starting with "/" or drive letters.
     *   - Throws std::invalid_argument("Error: Invalid file path.") if violated.
     * 
     * File Size Limit:
     *   - Checks via std::filesystem::file_size.
     *   - If > 1,048,576 bytes (1 MB): throw std::runtime_error("File exceeds maximum size limit.")
     * 
     * Behavior:
     *   - Reads full file content via std::ifstream.
     *   - Extracts filename from file_path using std::filesystem::path.
     *   - Captures old content if filename exists in working_directory (for undo).
     *   - Creates unique_ptr<ImportFileCommand>, executes, pushes to undo_stack.
     *   - Clears redo_stack.
     * 
     * @param repo Repository to modify.
     * @param file_path Path to external file (relative to current working directory).
     * @param caller Session performing the operation.
     * @throws std::runtime_error if detached HEAD or file too large or cannot be read.
     * @throws std::invalid_argument if file_path is invalid or contains path traversal.
     */
    void import_file(Repository& repo,
                     const std::string& file_path,
                     const Session& caller);

    /**
     * @brief Remove a file from the working directory.
     * 
     * Detached HEAD Check:
     *   - If caller.is_detached: throw std::runtime_error("Detached HEAD: read-only state.")
     * 
     * Behavior:
     *   - Verifies filename exists in working_directory.
     *   - Creates unique_ptr<RemoveFileCommand> (constructor captures filename + content).
     *   - Calls execute() to remove the file.
     *   - Pushes command to repo.undo_stack.
     *   - Clears redo_stack.
     * 
     * @param repo Repository to modify.
     * @param filename File to remove.
     * @param caller Session performing the operation.
     * @throws std::runtime_error if detached HEAD or file not found.
     */
    void remove_file(Repository& repo,
                     const std::string& filename,
                     const Session& caller);

    /**
     * @brief Undo the most recent uncommitted change.
     * 
     * Behavior:
     *   - Pops top unique_ptr<Command> from repo.undo_stack.
     *   - Calls undo() on the command.
     *   - Moves ownership to repo.redo_stack.
     * 
     * @param repo Repository to undo in.
     * @throws std::runtime_error if undo_stack is empty.
     */
    void undo(Repository& repo);

    /**
     * @brief Redo the most recently undone change.
     * 
     * Behavior:
     *   - Pops top unique_ptr<Command> from repo.redo_stack.
     *   - Calls execute() on the command.
     *   - Moves ownership to repo.undo_stack.
     * 
     * @param repo Repository to redo in.
     * @throws std::runtime_error if redo_stack is empty.
     */
    void redo(Repository& repo);

    /**
     * @brief Commit the current working directory state as an immutable snapshot.
     * 
     * Detached HEAD Check:
     *   - If caller.is_detached: throw std::runtime_error("Detached HEAD: read-only state.")
     * 
     * Validation:
     *   - If working_directory is empty: throw std::runtime_error("Error: Working directory is empty.")
     * 
     * Behavior:
     *   - Generates unique commit hash via generate_commit_hash().
     *   - Creates CommitNode with:
     *       hash = generated hash
     *       message = provided message
     *       timestamp = current datetime as formatted string
     *       parent_hash = repo.head_commit_hash (empty string if first commit)
     *       file_snapshot = deep copy of repo.working_directory
     *   - Inserts into repo.commit_history as unique_ptr<CommitNode>.
     *   - Updates repo.head_commit_hash = new node's hash.
     *   - Clears both repo.undo_stack and repo.redo_stack.
     * 
     * @param repo Repository to commit to.
     * @param message Commit message describing the snapshot.
     * @param caller Session performing the commit.
     * @throws std::runtime_error if detached HEAD or working directory empty.
     */
    void commit(Repository& repo,
                const std::string& message,
                const Session& caller);

    /**
     * @brief Restore working directory to a previous commit state (checkout).
     * 
     * Detached HEAD Transition:
     *   - If commit_hash_or_HEAD == "HEAD": return to attached HEAD (caller.is_detached = false).
     *   - Otherwise: enter detached HEAD state (caller.is_detached = true).
     * 
     * Dirty State Check (unless zero commits):
     *   - If repo.head_commit_hash is not empty (commits exist):
     *       Retrieve HEAD CommitNode from commit_history.
     *       Compare repo.working_directory against HEAD->file_snapshot.
     *       If not identical: throw std::runtime_error("Working directory not clean. Please commit or undo changes before checking out.")
     *   - If repo.head_commit_hash is empty (no commits): skip check.
     * 
     * Checkout Logic:
     *   - If commit_hash_or_HEAD == "HEAD":
     *       Set caller.is_detached = false.
     *       Restore working_directory from HEAD->file_snapshot.
     *       Return.
     *   - Otherwise:
     *       Lookup commit_hash_or_HEAD in commit_history.
     *       If not found: throw std::runtime_error("Commit not found.")
     *       Replace repo.working_directory with found node's file_snapshot.
     *       Set caller.is_detached = true.
     * 
     * @param repo Repository to checkout in.
     * @param commit_hash_or_HEAD Commit hash string or literal "HEAD".
     * @param caller Session (is_detached flag updated).
     * @throws std::runtime_error if working directory not clean or commit not found.
     */
    void checkout(Repository& repo,
                  const std::string& commit_hash_or_HEAD,
                  Session& caller);

    /**
     * @brief Reset working directory to HEAD state and clear all undo/redo commands.
     * 
     * Preconditions:
     *   - If caller.is_detached: throw std::runtime_error("Cannot reset in detached state.")
     * 
     * Behavior:
     *   - Clears repo.undo_stack and repo.redo_stack (unique_ptr destructors handle cleanup).
     *   - If repo.head_commit_hash is empty (no commits):
     *       Set working_directory to empty map and return.
     *   - Otherwise:
     *       Overwrites repo.working_directory with file_snapshot from HEAD CommitNode.
     * 
     * @param repo Repository to reset.
     * @param caller Session performing the reset.
     * @throws std::runtime_error if in detached HEAD state.
     */
    void reset(Repository& repo, const Session& caller);

    /**
     * @brief Retrieve the commit history log for the repository.
     * 
     * Permission Check:
     *   - If repo is private AND caller is not ADMIN AND caller.username != repo.owner_username:
     *     throw std::runtime_error("Repository not found.")
     * 
     * Behavior:
     *   - Traverses the commit chain backward from repo.head_commit_hash via parent_hash.
     *   - Stops when parent_hash == "" (reached root commit).
     *   - Collects all commits in newest-first order.
     *   - Formats each entry: "Hash: <hash> | Message: <msg> | Time: <ts>"
     * 
     * @param repo Repository to query.
     * @param caller Session performing the query.
     * @return Vector of formatted log entry strings, newest first.
     * @throws std::runtime_error if access denied or repository is private and caller unauthorized.
     */
    std::vector<std::string> get_log(const Repository& repo, const Session& caller) const;

    /**
     * @brief Get the current status of the working directory.
     * 
     * Behavior:
     *   - If working_directory is empty: return "Working directory is empty."
     *   - Otherwise: list all filenames with their line counts.
     *   - Format per line: "  <filename>: <line_count> lines"
     *   - Return as single formatted string.
     * 
     * @param repo Repository to query.
     * @return Status string describing staged files and line counts.
     */
    std::string get_status(const Repository& repo) const;

    /**
     * @brief View the contents of a staged file.
     * 
     * Behavior:
     *   - Lookup filename in repo.working_directory.
     *   - If not found: throw std::runtime_error("File not found in staging area.")
     *   - Split content by newline character.
     *   - Return as single formatted string with line numbers (1-indexed).
     *   - Format per line: "1: <content>\n2: <content>\n..."
     * 
     * @param repo Repository to query.
     * @param filename File to view.
     * @return Formatted string with numbered lines.
     * @throws std::runtime_error if file not found.
     */
    std::string view_file(const Repository& repo, const std::string& filename) const;
};
