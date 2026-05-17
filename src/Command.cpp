#include "Command.hpp"
#include <sstream>
#include <vector>

using namespace std;

// Helper function to split a string by newline delimiter
static vector<string> split_by_newline(const string& str) {
    vector<string> lines;
    if (str.empty()) {
        return lines;
    }
    
    stringstream ss(str);
    string line;
    while (getline(ss, line)) {
        lines.push_back(line);
    }
    
    return lines;
}

// Helper function to join a vector of strings by newline delimiter
static string join_by_newline(const vector<string>& lines) {
    if (lines.empty()) {
        return "";
    }
    
    string result;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) {
            result += "\n";
        }
        result += lines[i];
    }
    
    return result;
}

// ============================================================================
// AddLineCommand Implementation
// ============================================================================

AddLineCommand::AddLineCommand(unordered_map<string, string>& wdir,
                               const string& filename,
                               int line_number,
                               const string& text)
    : working_directory(wdir), filename(filename), line_number(line_number), text(text) {}

void AddLineCommand::execute() {
    // Check if file exists
    auto it = working_directory.find(filename);
    if (it == working_directory.end()) {
        throw runtime_error("File '" + filename + "' does not exist in working directory.");
    }

    // Split file content into lines
    vector<string> lines = split_by_newline(it->second);
    
    // Validate line_number: must be in range [1, lines.size() + 1]
    // (1-indexed, can insert at end)
    if (line_number < 1 || line_number > static_cast<int>(lines.size()) + 1) {
        throw invalid_argument("Line number " + to_string(line_number) + 
                                   " out of range. Valid range: [1, " + 
                                   to_string(lines.size() + 1) + "]");
    }

    // Insert text at position (line_number - 1) in the vector
    lines.insert(lines.begin() + (line_number - 1), text);

    // Rejoin and update working_directory
    it->second = join_by_newline(lines);
}

void AddLineCommand::undo() {
    // Get the file (should exist)
    auto it = working_directory.find(filename);
    if (it == working_directory.end()) {
        throw runtime_error("File '" + filename + "' no longer exists.");
    }

    // Split file content into lines
    vector<string> lines = split_by_newline(it->second);
    
    // Verify that line still exists (sanity check)
    if (line_number < 1 || line_number > static_cast<int>(lines.size())) {
        throw runtime_error("Cannot undo: line " + to_string(line_number) +
                                " no longer exists.");
    }

    // Remove the line at position (line_number - 1)
    lines.erase(lines.begin() + (line_number - 1));

    // Rejoin and update working_directory
    it->second = join_by_newline(lines);
}

// ============================================================================
// DeleteLineCommand Implementation
// ============================================================================

DeleteLineCommand::DeleteLineCommand(unordered_map<string, string>& wdir,
                                     const string& filename,
                                     int line_number)
    : working_directory(wdir), filename(filename), line_number(line_number), deleted_content("") {}

void DeleteLineCommand::execute() {
    // Check if file exists
    auto it = working_directory.find(filename);
    if (it == working_directory.end()) {
        throw runtime_error("File '" + filename + "' does not exist in working directory.");
    }

    // Split file content into lines
    vector<string> lines = split_by_newline(it->second);
    
    // Validate line_number: must be in range [1, lines.size()]
    if (line_number < 1 || line_number > static_cast<int>(lines.size())) {
        throw invalid_argument("Line number " + to_string(line_number) +
                                   " out of range. Valid range: [1, " + 
                                   to_string(lines.size()) + "]");
    }

    // Store the deleted line content for undo
    deleted_content = lines[line_number - 1];

    // Remove the line at position (line_number - 1)
    lines.erase(lines.begin() + (line_number - 1));

    // Rejoin and update working_directory
    it->second = join_by_newline(lines);
}

void DeleteLineCommand::undo() {
    // Get the file (should exist)
    auto it = working_directory.find(filename);
    if (it == working_directory.end()) {
        throw runtime_error("File '" + filename + "' no longer exists.");
    }

    // Split file content into lines
    vector<string> lines = split_by_newline(it->second);
    
    // Validate line_number for insertion: [1, lines.size() + 1]
    if (line_number < 1 || line_number > static_cast<int>(lines.size()) + 1) {
        throw runtime_error("Cannot undo: line number " + to_string(line_number) +
                                " is out of valid insertion range.");
    }

    // Re-insert the deleted line at its original position
    lines.insert(lines.begin() + (line_number - 1), deleted_content);

    // Rejoin and update working_directory
    it->second = join_by_newline(lines);
}

// ============================================================================
// ImportFileCommand Implementation
// ============================================================================

ImportFileCommand::ImportFileCommand(unordered_map<string, string>& wdir,
                                     const string& filename,
                                     const string& content)
    : working_directory(wdir), filename(filename), content(content), 
      existed_before(false), old_content("") {
    
    // Capture whether file existed and its old content
    auto it = working_directory.find(filename);
    if (it != working_directory.end()) {
        existed_before = true;
        old_content = it->second;
    }
}

void ImportFileCommand::execute() {
    // Insert or overwrite the file with new content
    working_directory[filename] = content;
}

void ImportFileCommand::undo() {
    if (existed_before) {
        // File existed before import, restore old content
        working_directory[filename] = old_content;
    } else {
        // File did not exist before import, remove it
        working_directory.erase(filename);
    }
}

// ============================================================================
// RemoveFileCommand Implementation
// ============================================================================

RemoveFileCommand::RemoveFileCommand(unordered_map<string, string>& wdir,
                                     const string& filename)
    : working_directory(wdir), filename(filename), stored_content("") {
    
    // Check that file exists in working_directory
    auto it = working_directory.find(filename);
    if (it == working_directory.end()) {
        throw invalid_argument("File '" + filename + 
                                   "' does not exist in working directory. Cannot remove.");
    }

    // Capture the full file content for restoration via undo
    stored_content = it->second;
}

void RemoveFileCommand::execute() {
    // Erase the file from working_directory
    working_directory.erase(filename);
}

void RemoveFileCommand::undo() {
    // Re-insert the file with its stored content
    working_directory[filename] = stored_content;
}
