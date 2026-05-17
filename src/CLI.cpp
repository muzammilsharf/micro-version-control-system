#include "CLI.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

using namespace std;

// ============================================================================
// Constructor
// ============================================================================

CLI::CLI() : current_session{"", Role::GUEST, false, false, ""} {
    // Session starts inactive
}

// ============================================================================
// Private Helper Methods
// ============================================================================

vector<string> CLI::parse_command(const string& line) const {
    vector<string> tokens;
    stringstream ss(line);
    string token;
    bool in_quotes = false;
    string current_token;

    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];

        if (c == '"') {
            in_quotes = !in_quotes;
        } else if (isspace(c) && !in_quotes) {
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
        } else {
            current_token += c;
        }
    }

    if (!current_token.empty()) {
        tokens.push_back(current_token);
    }

    return tokens;
}

void CLI::require_active_session() const {
    if (!current_session.is_active) {
        throw runtime_error("Not logged in.");
    }
}

void CLI::require_role(Role minimum) const {
    require_active_session();
    if (current_session.role < minimum) {
        throw runtime_error("Permission denied.");
    }
}

void CLI::print_help() const {
    cout << "\n=== Available Commands ===\n";
    cout << "help                                 - Show this help message\n";
    cout << "login <username> <password>          - Login with username and password\n";
    cout << "login guest                          - Login as guest (read-only)\n";
    cout << "logout                               - Logout from current session\n";

    if (!current_session.is_active || current_session.role == Role::GUEST) {
        cout << "register <username> <password>       - Register a new account\n";
    }

    if (current_session.is_active) {
        if (current_session.role == Role::ADMIN) {
            cout << "list_users                           - List all registered users (ADMIN)\n";
        }

        cout << "create_repo <name> <public|private>  - Create a new repository\n";
        cout << "delete_repo <name>                   - Delete a repository\n";
        cout << "list_repos                           - List your repositories\n";
        cout << "list_public_repos                    - List all public repositories\n";
        cout << "search <prefix>                      - Search public repositories by prefix\n";

        if (current_session.role != Role::GUEST) {
            cout << "log <repo>                           - View commit history\n";
            cout << "status <repo>                        - View staged files\n";
            cout << "view_file <repo> <filename>          - View file contents\n";
            cout << "import_file <repo> <filepath>        - Import external file\n";
            cout << "remove_file <repo> <filename>        - Remove file from staging\n";
            cout << "add_line <repo> <file> <line> <txt>  - Insert line in file\n";
            cout << "delete_line <repo> <file> <line>     - Delete line from file\n";
            cout << "undo <repo>                          - Undo last change\n";
            cout << "redo <repo>                          - Redo last undone change\n";
            cout << "reset <repo>                         - Reset to HEAD state\n";
            cout << "commit <repo> <message>              - Create commit snapshot\n";
            cout << "checkout <repo> <hash|HEAD>          - Restore commit state\n";
        }
    }

    cout << "exit                                 - Save and exit\n";
    cout << "========================\n\n";
}

bool CLI::has_uncommitted_changes() const {
    if (!current_session.is_active) {
        return false;
    }

    try {
        const auto& all_repos = repo_manager.get_all_repos();
        const auto& user = auth_manager.get_user(current_session.username);

        for (const auto& repo_name : user.repo_list) {
            auto it = all_repos.find(repo_name);
            if (it != all_repos.end()) {
                const auto& repo = it->second;

                // Check if working directory is dirty
                if (!repo.head_commit_hash.empty()) {
                    auto commit_it = repo.commit_history.find(repo.head_commit_hash);
                    if (commit_it != repo.commit_history.end()) {
                        const auto& head_snapshot = commit_it->second->file_snapshot;
                        if (repo.working_directory.size() != head_snapshot.size()) {
                            return true;
                        }

                        for (const auto& pair : repo.working_directory) {
                            auto snap_it = head_snapshot.find(pair.first);
                            if (snap_it == head_snapshot.end() || snap_it->second != pair.second) {
                                return true;
                            }
                        }
                    }
                } else if (!repo.working_directory.empty()) {
                    return true;  // Files staged but no commits yet
                }
            }
        }
    } catch (...) {
        return false;
    }

    return false;
}

// ============================================================================
// Command Handlers
// ============================================================================

void CLI::handle_login(const vector<string>& tokens) {
    if (tokens.size() < 2) {
    cout << "Usage: login <username> <password>\n       login guest\n";
    return;
}
    string username = tokens[1];
    string password = (tokens.size() >= 3) ? tokens[2] : "";

    current_session = auth_manager.login(username, password);
    cout << "Logged in as " << current_session.username << " (" << role_to_string(current_session.role) << ")\n";
    current_session.active_repo_name = "";
}

void CLI::handle_logout() {
    require_active_session();

    // Check for uncommitted changes
    if (has_uncommitted_changes()) {
        cout << "Uncommitted changes will be lost. Proceed? [y/N] ";
        string response;
        getline(cin, response);
        if (response != "y" && response != "Y") {
            cout << "Logout cancelled.\n";
            return;
        }
    }

    current_session.is_active = false;
    current_session.username = "";
    current_session.role = Role::GUEST;
    current_session.active_repo_name = "";
    current_session.is_detached = false;
    cout << "Logged out.\n";
}

void CLI::handle_register(const vector<string>& tokens) {
    if (tokens.size() < 3) {
        cout << "Usage: register <username> <password> [role]\n";
        return;
    }

    string username = tokens[1];
    string password = tokens[2];
    Role role = Role::USER;

    if (tokens.size() >= 4) {
        role = string_to_role(tokens[3]);
    }

    Session caller_session = current_session;
    if (!current_session.is_active) {
        caller_session.role = Role::GUEST;
    }

    auth_manager.register_user(username, password, role, caller_session);
    cout << "User '" << username << "' registered as " << role_to_string(role) << ".\n";
}

void CLI::handle_list_users() {
    require_role(Role::ADMIN);

    const auto& users = auth_manager.get_all_users();
    cout << "\n=== Registered Users ===\n";
    cout << "ID | Username | Role\n";
    cout << "---|----------|-------\n";
    for (const auto& pair : users) {
        const auto& user = pair.second;
        cout << user.id << " | " << user.username << " | " << role_to_string(user.role) << "\n";
    }
    cout << "=======================\n\n";
}

void CLI::handle_create_repo(const vector<string>& tokens) {
    require_active_session();

    if (tokens.size() < 3) {
        cout << "Usage: create_repo <name> <public|private>\n";
        return;
    }

    string name = tokens[1];
    string visibility = tokens[2];
    bool is_public = (visibility == "public");

    repo_manager.create_repo(name, is_public, current_session.username, auth_manager);
    cout << "Repository '" << name << "' created as " << visibility << ".\n";
}

void CLI::handle_delete_repo(const vector<string>& tokens) {
    require_active_session();

    if (tokens.size() < 2) {
        cout << "Usage: delete_repo <name>\n";
        return;
    }

    string name = tokens[1];
    repo_manager.delete_repo(name, current_session, auth_manager);
    cout << "Repository '" << name << "' deleted.\n";
}

void CLI::handle_list_repos() {
    require_active_session();

    auto repos = repo_manager.list_user_repos(current_session.username);
    cout << "\n=== Your Repositories ===\n";
    for (const auto& repo_name : repos) {
        try {
            const auto& repo = repo_manager.get_repo(repo_name, current_session);
            int commit_count = repo.commit_history.size();
            string visibility = repo.is_public ? "public" : "private";
            cout << "  " << repo_name << " (" << visibility << ", " << commit_count << " commits)\n";
        } catch (...) {
            // Silently skip inaccessible repos
        }
    }
    cout << "=========================\n\n";
}

void CLI::handle_list_public_repos() {
    auto repos = repo_manager.list_public_repos();
    cout << "\n=== Public Repositories ===\n";
    if (repos.empty()) {
        cout << "  No public repositories found.\n";
    } 
    else {
        // Create a temporary ADMIN session to bypass permission checks on public repos
        Session temp_session;
        temp_session.is_active = true;
        temp_session.role = Role::ADMIN;

        for (const auto& repo_name : repos) {
            try {
                const auto& repo = repo_manager.get_repo(repo_name, current_session);
                cout << "  " << repo_name << " (owner: " << repo.owner_username << ")\n";
            } catch (...) {
             // Silently skip inaccessible repos
            }
        }
    }
    cout << "============================\n\n";
}

void CLI::handle_search(const vector<string>& tokens) {
    if (tokens.size() < 2) {
        cout << "Usage: search <prefix>\n";
        return;
    }

    string prefix = tokens[1];
    auto results = repo_manager.search_repos(prefix);

    cout << "\n=== Search Results for '" << prefix << "' ===\n";
    if (results.empty()) {
        cout << "No repositories found.\n";
    } else {
        for (const auto& repo_name : results) {
            cout << "  " << repo_name << "\n";
        }
    }
    cout << "=========================================\n\n";
}

void CLI::handle_log(const vector<string>& tokens) {
    require_active_session();

    if (tokens.size() < 2) {
        cout << "Usage: log <repo>\n";
        return;
    }

    string repo_name = tokens[1];
    const auto& repo = repo_manager.get_repo(repo_name, current_session);
    auto log_entries = vcs_engine.get_log(repo, current_session);

    cout << "\n=== Commit History for '" << repo_name << "' ===\n";
    if (log_entries.empty()) {
        cout << "No commits yet.\n";
    } else {
        for (const auto& entry : log_entries) {
            cout << entry << "\n";
        }
    }
    cout << "===========================================\n\n";
}

void CLI::handle_status(const vector<string>& tokens) {
    require_active_session();

    if (tokens.size() < 2) {
        cout << "Usage: status <repo>\n";
        return;
    }

    string repo_name = tokens[1];
    const auto& repo = repo_manager.get_repo(repo_name, current_session);
    string status_str = vcs_engine.get_status(repo);

    cout << "\n=== Status for '" << repo_name << "' ===\n";
    cout << status_str << "\n";
    cout << "======================================\n\n";
}

void CLI::handle_view_file(const vector<string>& tokens) {
    require_active_session();

    if (tokens.size() < 3) {
        cout << "Usage: view_file <repo> <filename>\n";
        return;
    }

    string repo_name = tokens[1];
    string filename = tokens[2];
    const auto& repo = repo_manager.get_repo(repo_name, current_session);
    string content = vcs_engine.view_file(repo, filename);

    cout << "\n=== " << filename << " ===\n";
    cout << content << "\n";
    cout << "==========================\n\n";
}

void CLI::handle_import_file(const vector<string>& tokens) {
    require_active_session();

    if (tokens.size() < 3) {
        cout << "Usage: import_file <repo> <filepath>\n";
        return;
    }

    string repo_name = tokens[1];
    string file_path = tokens[2];
    auto& repo = repo_manager.get_repo(repo_name, current_session);

    vcs_engine.import_file(repo, file_path, current_session);
    cout << "File imported successfully.\n";
}

void CLI::handle_remove_file(const vector<string>& tokens) {
    require_active_session();

    if (tokens.size() < 3) {
        cout << "Usage: remove_file <repo> <filename>\n";
        return;
    }

    string repo_name = tokens[1];
    string filename = tokens[2];
    auto& repo = repo_manager.get_repo(repo_name, current_session);

    vcs_engine.remove_file(repo, filename, current_session);
    cout << "File removed from staging.\n";
}

void CLI::handle_add_line(const vector<string>& tokens) {
    require_active_session();

    if (tokens.size() < 5) {
        cout << "Usage: add_line <repo> <filename> <line_number> <text>\n";
        return;
    }

    string repo_name = tokens[1];
    string filename = tokens[2];
    int line_number;
    try{
        line_number = stoi(tokens[3]);
    }catch(...){
        cout << "Error: line_number must be a valid integer.\n";
        return;
    }
    string text = tokens[4];

    auto& repo = repo_manager.get_repo(repo_name, current_session);
    vcs_engine.add_line(repo, filename, line_number, text, current_session);
    cout << "Line added.\n";
}

void CLI::handle_delete_line(const vector<string>& tokens) {
    require_active_session();

    if (tokens.size() < 4) {
        cout << "Usage: delete_line <repo> <filename> <line_number>\n";
        return;
    }

    string repo_name = tokens[1];
    string filename = tokens[2];
    int line_number;
    try{
        line_number = stoi(tokens[3]);
    }catch(...){
        cout << "Error: line_number must be a valid integer.\n";
        return;
    }

    auto& repo = repo_manager.get_repo(repo_name, current_session);
    vcs_engine.delete_line(repo, filename, line_number, current_session);
    cout << "Line deleted.\n";
}

void CLI::handle_undo(const vector<string>& tokens) {
    require_active_session();

    if (tokens.size() < 2) {
        cout << "Usage: undo <repo>\n";
        return;
    }

    string repo_name = tokens[1];
    auto& repo = repo_manager.get_repo(repo_name, current_session);

    vcs_engine.undo(repo);
    cout << "Undo completed.\n";
}

void CLI::handle_redo(const vector<string>& tokens) {
    require_active_session();

    if (tokens.size() < 2) {
        cout << "Usage: redo <repo>\n";
        return;
    }

    string repo_name = tokens[1];
    auto& repo = repo_manager.get_repo(repo_name, current_session);

    vcs_engine.redo(repo);
    cout << "Redo completed.\n";
}

void CLI::handle_reset(const vector<string>& tokens) {
    require_active_session();

    if (tokens.size() < 2) {
        cout << "Usage: reset <repo>\n";
        return;
    }

    string repo_name = tokens[1];
    auto& repo = repo_manager.get_repo(repo_name, current_session);

    vcs_engine.reset(repo, current_session);
    cout << "Reset to HEAD state.\n";
}

void CLI::handle_commit(const vector<string>& tokens) {
    require_active_session();

    if (tokens.size() < 3) {
        cout << "Usage: commit <repo> <message>\n";
        return;
    }

    string repo_name = tokens[1];
    string message = tokens[2];

    auto& repo = repo_manager.get_repo(repo_name, current_session);
    vcs_engine.commit(repo, message, current_session);
    cout << "Commit created.\n";
}

void CLI::handle_checkout(const vector<string>& tokens) {
    require_active_session();

    if (tokens.size() < 3) {
        cout << "Usage: checkout <repo> <commit_hash|HEAD>\n";
        return;
    }

    string repo_name = tokens[1];
    string commit_hash_or_head = tokens[2];

    auto& repo = repo_manager.get_repo(repo_name, current_session);
    vcs_engine.checkout(repo, commit_hash_or_head, current_session);

    if (commit_hash_or_head == "HEAD") {
        cout << "Returned to latest commit.\n";
    } else {
        cout << "Detached HEAD state. Use 'checkout HEAD' to return.\n";
    }
}

void CLI::handle_exit() {
    // Check for uncommitted changes
    if (has_uncommitted_changes()) {
        cout << "Uncommitted changes will be lost. Proceed? [y/N] ";
        string response;
        getline(cin, response);
        if (response != "y" && response != "Y") {
            cout << "Exit cancelled.\n";
            return;
        }
    }

    try {
        const auto& all_users = auth_manager.get_all_users();
        const auto& all_repos = repo_manager.get_all_repos();
        persistence_manager.save_all_state(all_users, all_repos);
        cout << "All changes saved. Goodbye!\n";
    } catch (const exception& e) {
        cout << "Error saving state: " << e.what() << "\n";
    }
}

// ============================================================================
// Public Run Method
// ============================================================================

void CLI::run() {
    // Load persisted state
    unordered_map<string, User> loaded_users;
    unordered_map<string, Repository> loaded_repos;

    try {
        persistence_manager.load_all_state(loaded_users, loaded_repos);
        auth_manager.load_users(loaded_users);
        for (auto& repo_pair : loaded_repos) {
            Repository& repo = repo_pair.second;
            if (!repo.head_commit_hash.empty()) {
                auto it = repo.commit_history.find(repo.head_commit_hash);
                if (it != repo.commit_history.end()) {
                    repo.working_directory = it->second->file_snapshot;
                }
            }
        }
        repo_manager.load_repos(move(loaded_repos));
    } catch (const exception& e) {
        cout << "Error loading state: " << e.what() << "\n";
    }

    cout << "\n=== Micro-VCS Terminal ===\n";
    cout << "Type 'help' for a list of commands.\n\n";

    string line;
    while (true) {
        cout << "> ";
        if (!getline(cin, line)) {
            break;  // EOF
        }

        if (line.empty()) {
            continue;
        }

        // Trim leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty()) {
            continue;
        }

        auto tokens = parse_command(line);
        if (tokens.empty()) {
            continue;
        }

        try {
            const string& command = tokens[0];

            if (command == "help") {
                print_help();
            } else if (command == "login") {
                handle_login(tokens);
            } else if (command == "logout") {
                handle_logout();
            } else if (command == "register") {
                handle_register(tokens);
            } else if (command == "list_users") {
                handle_list_users();
            } else if (command == "create_repo") {
                handle_create_repo(tokens);
            } else if (command == "delete_repo") {
                handle_delete_repo(tokens);
            } else if (command == "list_repos") {
                handle_list_repos();
            } else if (command == "list_public_repos") {
                handle_list_public_repos();
            } else if (command == "search") {
                handle_search(tokens);
            } else if (command == "log") {
                handle_log(tokens);
            } else if (command == "status") {
                handle_status(tokens);
            } else if (command == "view_file") {
                handle_view_file(tokens);
            } else if (command == "import_file") {
                handle_import_file(tokens);
            } else if (command == "remove_file") {
                handle_remove_file(tokens);
            } else if (command == "add_line") {
                handle_add_line(tokens);
            } else if (command == "delete_line") {
                handle_delete_line(tokens);
            } else if (command == "undo") {
                handle_undo(tokens);
            } else if (command == "redo") {
                handle_redo(tokens);
            } else if (command == "reset") {
                handle_reset(tokens);
            } else if (command == "commit") {
                handle_commit(tokens);
            } else if (command == "checkout") {
                handle_checkout(tokens);
            } else if (command == "exit") {
                handle_exit();
                break;
            } else {
                cout << "Unknown command: " << command << "\n";
            }
        } catch (const exception& e) {
            cout << "Error: " << e.what() << "\n";
        }
    }
}
