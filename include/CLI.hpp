#pragma once

#include "Types.hpp"
#include "AuthManager.hpp"
#include "RepoManager.hpp"
#include "VCSEngine.hpp"
#include "PersistenceManager.hpp"
#include <vector>
#include <string>

/**
 * @class CLI
 * @brief Terminal REPL interface that connects all manager classes and handles user I/O.
 *
 * The CLI is the ONLY layer responsible for input/output and exception handling.
 * All business logic lives in the manager classes; the CLI is purely a dispatcher.
 * Exceptions from managers are caught and formatted as user-friendly messages.
 */
class CLI {
private:
    /**
     * @brief Manager for user authentication and registration.
     */
    AuthManager auth_manager;

    /**
     * @brief Manager for repository CRUD operations and Trie indexing.
     */
    RepoManager repo_manager;

    /**
     * @brief Engine for version control operations (commits, checkout, undo/redo).
     */
    VCSEngine vcs_engine;

    /**
     * @brief Manager for persistent file I/O (atomic writes, serialization).
     */
    PersistenceManager persistence_manager;

    /**
     * @brief Current active session (login state, role, detached HEAD flag).
     */
    Session current_session;

    /**
     * @brief Parse a command line into tokens, respecting quoted strings.
     * 
     * Splits by whitespace, but treats quoted strings ("...") as single tokens.
     * Example: commit myrepo "my message" -> ["commit", "myrepo", "my message"]
     * 
     * @param line The raw command line input.
     * @return Vector of parsed tokens.
     */
    std::vector<std::string> parse_command(const std::string& line) const;

    /**
     * @brief Verify that a user is logged in (session is active).
     * 
     * @throws std::runtime_error("Not logged in.") if not authenticated.
     */
    void require_active_session() const;

    /**
     * @brief Verify that the current user's role meets the minimum required level.
     * 
     * Role hierarchy: ADMIN > USER > GUEST
     * 
     * @param minimum The minimum role required (ADMIN, USER, or GUEST).
     * @throws std::runtime_error("Permission denied.") if role is insufficient.
     */
    void require_role(Role minimum) const;

    /**
     * @brief Print the help menu, filtered by the current user's role.
     * 
     * Lists all available commands that the current role is authorized to execute.
     */
    void print_help() const;

    /**
     * @brief Check if any of the current user's repositories have uncommitted changes.
     * 
     * Compares working_directory against HEAD snapshot for each repository.
     * 
     * @return true if any repository has uncommitted changes, false otherwise.
     */
    bool has_uncommitted_changes() const;

    // ========================================================================
    // Command Handler Methods
    // ========================================================================

    /**
     * @brief Handle "login <username> <password>" or "login guest".
     * 
     * Authenticates the user and starts a session.
     * Usage: login <username> <password>
     *        login guest
     */
    void handle_login(const std::vector<std::string>& tokens);

    /**
     * @brief Handle "logout" command.
     * 
     * Checks for uncommitted changes and prompts before terminating session.
     */
    void handle_logout();

    /**
     * @brief Handle "register <username> <password> [role]" or "register <username> <password>".
     * 
     * Registers a new user. Only ADMINs can specify a role; others default to USER.
     * Usage: register newuser password123
     *        register newuser password123 ADMIN  (ADMIN only)
     */
    void handle_register(const std::vector<std::string>& tokens);

    /**
     * @brief Handle "list_users" command (ADMIN only).
     * 
     * Lists all registered users and their roles.
     */
    void handle_list_users();

    /**
     * @brief Handle "create_repo <name> <public|private>" command.
     * 
     * Creates a new repository owned by the current user.
     * Usage: create_repo myrepo public
     *        create_repo secret private
     */
    void handle_create_repo(const std::vector<std::string>& tokens);

    /**
     * @brief Handle "delete_repo <name>" command.
     * 
     * Deletes a repository (owner or ADMIN only).
     * Usage: delete_repo myrepo
     */
    void handle_delete_repo(const std::vector<std::string>& tokens);

    /**
     * @brief Handle "list_repos" command.
     * 
     * Lists repositories owned by the current user.
     */
    void handle_list_repos();

    /**
     * @brief Handle "list_public_repos" command.
     * 
     * Lists all public repositories in the system.
     */
    void handle_list_public_repos();

    /**
     * @brief Handle "search <prefix>" command.
     * 
     * Searches for public repositories by name prefix (Trie-based).
     * Usage: search my
     */
    void handle_search(const std::vector<std::string>& tokens);

    /**
     * @brief Handle "log <repo>" command.
     * 
     * Displays commit history for a repository (newest first).
     * Usage: log myrepo
     */
    void handle_log(const std::vector<std::string>& tokens);

    /**
     * @brief Handle "status <repo>" command.
     * 
     * Shows all staged files and their line counts.
     * Usage: status myrepo
     */
    void handle_status(const std::vector<std::string>& tokens);

    /**
     * @brief Handle "view_file <repo> <filename>" command.
     * 
     * Displays file contents with line numbers.
     * Usage: view_file myrepo main.cpp
     */
    void handle_view_file(const std::vector<std::string>& tokens);

    /**
     * @brief Handle "import_file <repo> <filepath>" command.
     * 
     * Imports an external file into the staging area.
     * Usage: import_file myrepo /path/to/source.cpp
     */
    void handle_import_file(const std::vector<std::string>& tokens);

    /**
     * @brief Handle "remove_file <repo> <filename>" command.
     * 
     * Removes a file from the staging area.
     * Usage: remove_file myrepo main.cpp
     */
    void handle_remove_file(const std::vector<std::string>& tokens);

    /**
     * @brief Handle "add_line <repo> <filename> <line_number> <text>" command.
     * 
     * Inserts a line of text at the specified position.
     * Usage: add_line myrepo main.cpp 5 "new line content"
     */
    void handle_add_line(const std::vector<std::string>& tokens);

    /**
     * @brief Handle "delete_line <repo> <filename> <line_number>" command.
     * 
     * Removes a line from a file.
     * Usage: delete_line myrepo main.cpp 5
     */
    void handle_delete_line(const std::vector<std::string>& tokens);

    /**
     * @brief Handle "undo <repo>" command.
     * 
     * Reverses the most recent uncommitted change.
     * Usage: undo myrepo
     */
    void handle_undo(const std::vector<std::string>& tokens);

    /**
     * @brief Handle "redo <repo>" command.
     * 
     * Re-applies the most recently undone change.
     * Usage: redo myrepo
     */
    void handle_redo(const std::vector<std::string>& tokens);

    /**
     * @brief Handle "reset <repo>" command.
     * 
     * Clears all undo/redo commands and restores HEAD state.
     * Usage: reset myrepo
     */
    void handle_reset(const std::vector<std::string>& tokens);

    /**
     * @brief Handle "commit <repo> <message>" command.
     * 
     * Creates an immutable snapshot of the staging area.
     * Usage: commit myrepo "initial commit"
     */
    void handle_commit(const std::vector<std::string>& tokens);

    /**
     * @brief Handle "checkout <repo> <commit_hash_or_HEAD>" command.
     * 
     * Restores working directory to a previous commit or HEAD.
     * Usage: checkout myrepo abc123def456
     *        checkout myrepo HEAD
     */
    void handle_checkout(const std::vector<std::string>& tokens);

    /**
     * @brief Handle "exit" command.
     * 
     * Serializes all state and terminates the application.
     * Checks for uncommitted changes before exiting.
     */
    void handle_exit();

public:
    /**
     * @brief Constructor: initializes all managers and the session.
     */
    CLI();

    /**
     * @brief Run the main REPL loop.
     * 
     * Entry point for the terminal interface:
     *   1. Load all persisted state from disk.
     *   2. Print welcome message.
     *   3. Loop: read command, parse, dispatch to handler, catch exceptions.
     *   4. Exit on "exit" command or EOF.
     */
    void run();
};
