#include "PersistenceManager.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <ctime>
#include <iomanip>

// ============================================================================
// Private Helper Methods
// ============================================================================

std::string PersistenceManager::escape(const std::string& s) const {
    std::string result = s;
    
    // "Step 1: Replace \ with \\"
    size_t pos = 0;
    while ((pos = result.find('\\', pos)) != std::string::npos) {
        result.replace(pos, 1, "\\\\");
        pos += 2;  // Skip past the replacement
    }
    
    // "Step 2: Replace | with \|`
    pos = 0;
    while ((pos = result.find('|', pos)) != std::string::npos) {
        result.replace(pos, 1, "\\|");
        pos += 2;  // Skip past the replacement
    }
    
    return result;
}

std::string PersistenceManager::unescape(const std::string& s) const {
    std::string result = s;
    
    // Step 1: Replace \| with |
    size_t pos = 0;
    while ((pos = result.find("\\|", pos)) != std::string::npos) {
        result.replace(pos, 2, "|");
        pos += 1;  // Move past the replacement
    }
    
    // Step 2: Replace \\ with \|
    pos = 0;
    while ((pos = result.find("\\\\", pos)) != std::string::npos) {
        result.replace(pos, 2, "\\");
        pos += 1;  // Move past the replacement
    }
    
    return result;
}

std::vector<std::string> PersistenceManager::split_fields(const std::string& line) const {
    std::vector<std::string> fields;
    std::string current_field;
    
    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];
        
        // Check for escaped pipe: \|
        if (c == '\\' && i + 1 < line.length() && line[i + 1] == '|') {
            current_field += "\\|";
            ++i;  // Skip the next character
        }
        // Check for field delimiter
        else if (c == '|') {
            fields.push_back(current_field);
            current_field.clear();
        }
        // Regular character
        else {
            current_field += c;
        }
    }
    
    // Add the last field
    fields.push_back(current_field);
    
    return fields;
}

void PersistenceManager::atomic_write(const std::string& final_path, const std::string& content) const {
    std::string tmp_path = final_path + ".tmp";
    
    // Write to temporary file
    std::ofstream tmp_file(tmp_path);
    if (!tmp_file.is_open()) {
        throw std::runtime_error("Cannot open temporary file: " + tmp_path);
    }
    
    tmp_file << content;
    tmp_file.flush();
    tmp_file.close();
    
    // Atomic rename
    std::filesystem::rename(tmp_path, final_path);
}

// ============================================================================
// Constructor
// ============================================================================

PersistenceManager::PersistenceManager() {
    // Ensure data directory exists
    if (!std::filesystem::exists(DATA_DIR)) {
        std::filesystem::create_directories(DATA_DIR);
    }
}

// ============================================================================
// Public Methods
// ============================================================================

void PersistenceManager::save_all_state(const std::unordered_map<std::string, User>& users,
                                        const std::unordered_map<std::string, Repository>& repos) const {
    // Ensure data directory exists
    if (!std::filesystem::exists(DATA_DIR)) {
        std::filesystem::create_directories(DATA_DIR);
    }

    // ========================================================================
    // Serialize users.txt
    // ========================================================================
    std::stringstream users_stream;
    for (const auto& pair : users) {
        const User& user = pair.second;
        
        // Build repo_list as comma-separated escaped names
        std::string repo_list_str;
        for (size_t i = 0; i < user.repo_list.size(); ++i) {
            if (i > 0) repo_list_str += ",";
            repo_list_str += escape(user.repo_list[i]);
        }
        
        // Format: id|username|hashed_password|role_string|repo_list
        users_stream << user.id << "|"
                     << escape(user.username) << "|"
                     << escape(user.hashed_password) << "|"
                     << role_to_string(user.role) << "|"
                     << repo_list_str << "\n";
    }
    
    atomic_write(std::string(DATA_DIR) + "/users.txt", users_stream.str());

    // ========================================================================
    // Serialize repos.txt
    // ========================================================================
    std::stringstream repos_stream;
    for (const auto& pair : repos) {
        const Repository& repo = pair.second;
        
        // Format: id|name|owner_username|is_public(0/1)|head_commit_hash
        repos_stream << repo.id << "|"
                     << escape(repo.name) << "|"
                     << escape(repo.owner_username) << "|"
                     << (repo.is_public ? "1" : "0") << "|"
                     << escape(repo.head_commit_hash) << "\n";
    }
    
    atomic_write(std::string(DATA_DIR) + "/repos.txt", repos_stream.str());

    // ========================================================================
    // Serialize commits.txt
    // ========================================================================
    std::stringstream commits_stream;
    for (const auto& repo_pair : repos) {
        const Repository& repo = repo_pair.second;
        
        for (const auto& commit_pair : repo.commit_history) {
            const auto& node = commit_pair.second;
            
            // Build file snapshot string: filename1:content1||filename2:content2||...
            std::string snapshot_str;
            bool first = true;
            for (const auto& file_pair : node->file_snapshot) {
                if (!first) snapshot_str += "||";
                first = false;
                
                snapshot_str += escape(file_pair.first) + ":" + escape(file_pair.second);
            }
            
            // Format: repo_name|hash|parent_hash|timestamp|message|snapshot
            commits_stream << escape(repo.name) << "|"
                           << escape(node->hash) << "|"
                           << escape(node->parent_hash) << "|"
                           << escape(node->timestamp) << "|"
                           << escape(node->message) << "|"
                           << snapshot_str << "\n";
        }
    }
    
    atomic_write(std::string(DATA_DIR) + "/commits.txt", commits_stream.str());
}

void PersistenceManager::load_all_state(std::unordered_map<std::string, User>& out_users,
                                        std::unordered_map<std::string, Repository>& out_repos) const {
    // Ensure data directory exists
    if (!std::filesystem::exists(DATA_DIR)) {
        std::filesystem::create_directories(DATA_DIR);
        return;  // Empty state
    }

    // Clean up any stray .tmp files from interrupted writes
    try {
        for (const auto& entry : std::filesystem::directory_iterator(DATA_DIR)) {
            if (entry.path().extension() == ".tmp") {
                std::filesystem::remove(entry.path());
            }
        }
    } catch (...) {
        // Ignore errors during cleanup
    }

    // ========================================================================
    // Load users.txt
    // ========================================================================
    std::string users_path = std::string(DATA_DIR) + "/users.txt";
    if (std::filesystem::exists(users_path)) {
        std::ifstream users_file(users_path);
        std::string line;
        int line_num = 0;
        
        while (std::getline(users_file, line)) {
            ++line_num;
            if (line.empty()) continue;
            
            try {
                auto fields = split_fields(line);
                if (fields.size() != 5) {
                    std::cout << "WARNING: Skipping corrupted user record at line " << line_num 
                              << " (expected 5 fields, got " << fields.size() << ").\n";
                    continue;
                }
                
                User user;
                user.id = unescape(fields[0]);
                user.username = unescape(fields[1]);
                user.hashed_password = unescape(fields[2]);
                user.role = string_to_role(fields[3]);
                
                // Parse repo_list (comma-separated escaped names)
                user.repo_list.clear();
                if (!fields[4].empty()) {
                    std::stringstream ss(fields[4]);
                    std::string repo_name;
                    while (std::getline(ss, repo_name, ',')) {
                        user.repo_list.push_back(unescape(repo_name));
                    }
                }
                
                out_users[user.username] = user;
            } catch (const std::exception& e) {
                std::cout << "WARNING: Skipping corrupted user record at line " << line_num 
                          << " (" << e.what() << ").\n";
            }
        }
    }

    // ========================================================================
    // Load repos.txt
    // ========================================================================
    std::string repos_path = std::string(DATA_DIR) + "/repos.txt";
    if (std::filesystem::exists(repos_path)) {
        std::ifstream repos_file(repos_path);
        std::string line;
        int line_num = 0;
        
        while (std::getline(repos_file, line)) {
            ++line_num;
            if (line.empty()) continue;
            
            try {
                auto fields = split_fields(line);
                if (fields.size() != 5) {
                    std::cout << "WARNING: Skipping corrupted repo record at line " << line_num 
                              << " (expected 5 fields, got " << fields.size() << ").\n";
                    continue;
                }
                
                Repository repo;
                repo.id = unescape(fields[0]);
                repo.name = unescape(fields[1]);
                repo.owner_username = unescape(fields[2]);
                repo.is_public = (fields[3] == "1");
                repo.head_commit_hash = unescape(fields[4]);
                
                out_repos[repo.name] = std::move(repo);
            } catch (const std::exception& e) {
                std::cout << "WARNING: Skipping corrupted repo record at line " << line_num 
                          << " (" << e.what() << ").\n";
            }
        }
    }

    // ========================================================================
    // Load commits.txt
    // ========================================================================
    std::string commits_path = std::string(DATA_DIR) + "/commits.txt";
    if (std::filesystem::exists(commits_path)) {
        std::ifstream commits_file(commits_path);
        std::string line;
        int line_num = 0;
        
        while (std::getline(commits_file, line)) {
            ++line_num;
            if (line.empty()) continue;
            
            try {
                auto fields = split_fields(line);
                if (fields.size() != 6) {
                    std::cout << "WARNING: Skipping corrupted commit record at line " << line_num 
                              << " (expected 6 fields, got " << fields.size() << ").\n";
                    continue;
                }
                
                std::string repo_name = unescape(fields[0]);
                std::string hash = unescape(fields[1]);
                std::string parent_hash = unescape(fields[2]);
                std::string timestamp = unescape(fields[3]);
                std::string message = unescape(fields[4]);
                std::string snapshot_str = fields[5];
                
                // Find the repository
                auto repo_it = out_repos.find(repo_name);
                if (repo_it == out_repos.end()) {
                    std::cout << "WARNING: Skipping commit for unknown repository: " << repo_name << "\n";
                    continue;
                }
                
                // Create CommitNode
                auto node = std::make_unique<CommitNode>();
                node->hash = hash;
                node->message = message;
                node->timestamp = timestamp;
                node->parent_hash = parent_hash;
                
                // Parse file snapshot: filename1:content1||filename2:content2||...
                if (!snapshot_str.empty()) {
                    // Split by ||
                    size_t pos = 0;
                    while (pos < snapshot_str.length()) {
                        // Find next || or end
                        size_t next_pos = snapshot_str.find("||", pos);
                        if (next_pos == std::string::npos) {
                            next_pos = snapshot_str.length();
                        }
                        
                        std::string file_entry = snapshot_str.substr(pos, next_pos - pos);
                        
                        // Split by : to get filename and content
                        size_t colon_pos = file_entry.find(':');
                        if (colon_pos != std::string::npos) {
                            std::string filename = unescape(file_entry.substr(0, colon_pos));
                            std::string content = unescape(file_entry.substr(colon_pos + 1));
                            node->file_snapshot[filename] = content;
                        }
                        
                        pos = next_pos + 2;  // Skip the ||
                    }
                }
                
                // Insert into repository's commit_history
                repo_it->second.commit_history[hash] = std::move(node);
            } catch (const std::exception& e) {
                std::cout << "WARNING: Skipping corrupted commit record at line " << line_num 
                          << " (" << e.what() << ").\n";
            }
        }
    }
}