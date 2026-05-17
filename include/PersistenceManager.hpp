#pragma once

#include "Types.hpp"
#include <unordered_map>
#include <vector>
#include <string>

/**
 * @class PersistenceManager
 * @brief Handles all file I/O with atomic writes and delimiter escaping.
 *
 * Serializes Users_HashMap, Global_Repo_Map, and Commit Chain to flat files
 * with proper escaping for delimiters and newlines. Deserializes on startup
 * with fault tolerance for corrupted records.
 *
 * All file operations use atomic writes (write to .tmp, then rename) to prevent
 * data corruption on interrupted sessions.
 */
class PersistenceManager {
private:
    /**
     * @brief Path to the data directory where all persistent files are stored.
     */
    static constexpr const char* DATA_DIR = "data";

    /**
     * @brief Escape a string for safe serialization.
     * 
     * Escaping Order (CRITICAL):
     *   1. Replace all \ with \\
     *   2. Replace all | with \|
     * 
     * This ensures that unescaping (reverse order) produces the original string.
     * 
     * @param s The string to escape.
     * @return Escaped string safe for | delimiter and \ inclusion.
     */
    std::string escape(const std::string& s) const;

    /**
     * @brief Unescape a string after deserialization.
     * 
     * Unescaping Order (CRITICAL — reverse of escape):
     *   1. Replace all \| with |
     *   2. Replace all \\ with \
     * 
     * @param s The escaped string to unescape.
     * @return Original unescaped string.
     */
    std::string unescape(const std::string& s) const;

    /**
     * @brief Split a line by | delimiter, respecting \| as escaped literal.
     * 
     * Walks character by character. If current is \ and next is |, treats as
     * a literal escaped pipe (not a field boundary). Otherwise, | splits fields.
     * 
     * @param line The delimited line to split.
     * @return Vector of field strings (still escaped; caller must unescape if needed).
     */
    std::vector<std::string> split_fields(const std::string& line) const;

    /**
     * @brief Atomically write content to a file.
     * 
     * Atomic Write Strategy:
     *   1. Write content to final_path + ".tmp"
     *   2. Flush and close the stream.
     *   3. Call std::filesystem::rename(tmp_path, final_path) — atomic at OS level.
     * 
     * This ensures that even if the process is interrupted, the original file
     * remains uncorrupted (either old version or new version, never partial).
     * 
     * @param final_path The final destination path (e.g., "data/users.txt").
     * @param content The content to write.
     * @throws std::runtime_error if write or rename fails.
     */
    void atomic_write(const std::string& final_path, const std::string& content) const;

public:
    /**
     * @brief Constructor: initializes the PersistenceManager.
     */
    PersistenceManager();

    /**
     * @brief Save all in-memory state to disk atomically.
     * 
     * Writes three files in atomic fashion:
     * 
     * **data/users.txt**
     *   - Format: id|username|hashed_password|role_string|repo1,repo2,...
     *   - repo_list: comma-separated, each repo name escaped
     *   - One line per user
     * 
     * **data/repos.txt**
     *   - Format: id|name|owner_username|is_public(0/1)|head_commit_hash
     *   - name and owner_username are escaped
     *   - One line per repository
     *   - working_directory NOT serialized (transient session state)
     * 
     * **data/commits.txt**
     *   - Format: repo_name|hash|parent_hash|timestamp|message|snapshot
     *   - snapshot: filename1:content1||filename2:content2||...
     *   - Each filename and content escaped separately, files joined by ||
     *   - One line per CommitNode
     * 
     * All three files are written atomically via atomic_write().
     * 
     * @param users Map of username → User (from AuthManager).
     * @param repos Map of repo name → Repository (from RepoManager).
     */
    void save_all_state(const std::unordered_map<std::string, User>& users,
                        const std::unordered_map<std::string, Repository>& repos) const;

    /**
     * @brief Load all persisted state from disk.
     * 
     * Fault Tolerance:
     *   - If data/ directory does not exist: creates it, returns empty maps.
     *   - Scans data/ for .tmp files (from interrupted writes); deletes them.
     *   - Parses each file line by line. On malformed lines:
     *       1. Logs a non-fatal WARNING to stdout.
     *       2. Skips the corrupt record.
     *       3. Continues parsing remaining valid lines.
     *   - Validates field counts: users (5), repos (5), commits (6).
     *   - Under NO circumstances throws exceptions to caller.
     * 
     * **Head Commit Detection:**
     *   - After loading all commits: determines each repo's HEAD via:
     *       Traverse commit chain backward from root (parent_hash == "").
     *       The most recent commit (highest timestamp) is HEAD.
     *       Or: find the commit with no children (no other commit has this as parent_hash).
     * 
     * @param out_users Output map to populate with deserialized users.
     * @param out_repos Output map to populate with deserialized repositories.
     */
    void load_all_state(std::unordered_map<std::string, User>& out_users,
                        std::unordered_map<std::string, Repository>& out_repos) const;
};
