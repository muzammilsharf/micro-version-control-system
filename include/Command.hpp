#pragma once

#include <string>
#include <unordered_map>
#include <stdexcept>

/**
 * @class Command
 * @brief Abstract base class for the Command Pattern, enabling undo/redo functionality.
 *
 * All concrete command classes inherit from this interface and implement execute()
 * to perform an operation on a Repository's working_directory, and undo() to reverse it.
 */
class Command {
public:
    /**
     * @brief Execute the command operation.
     * @throws std::invalid_argument if the command parameters are invalid
     * @throws std::runtime_error if the operation cannot be performed
     */
    virtual void execute() = 0;

    /**
     * @brief Reverse the effects of execute().
     * @throws std::runtime_error if the undo operation cannot be performed
     */
    virtual void undo() = 0;

    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~Command() = default;
};

/**
 * @class AddLineCommand
 * @brief Command to insert a line of text at a specific line number in a file.
 *
 * Splits the file content by newline, inserts the text at the specified position
 * (1-indexed), and rejoins the content. undo() removes the inserted line.
 */
class AddLineCommand : public Command {
private:
    std::unordered_map<std::string, std::string>& working_directory;
    std::string filename;
    int line_number;
    std::string text;

public:
    /**
     * @brief Constructor for AddLineCommand.
     * @param wdir Reference to the Repository's working_directory map
     * @param filename Name of the file to edit
     * @param line_number 1-indexed line number where text will be inserted
     * @param text The text to insert
     */
    AddLineCommand(std::unordered_map<std::string, std::string>& wdir,
                   const std::string& filename,
                   int line_number,
                   const std::string& text);

    /**
     * @brief Insert the text at the specified line number.
     * @throws std::invalid_argument if line_number < 1 or > current_lines + 1
     * @throws std::runtime_error if the file does not exist in working_directory
     */
    void execute() override;

    /**
     * @brief Remove the line that was inserted.
     */
    void undo() override;
};

/**
 * @class DeleteLineCommand
 * @brief Command to remove a line from a file at a specific line number.
 *
 * Splits the file content by newline, removes the line at the specified position
 * (1-indexed), and rejoins the content. undo() restores the deleted line.
 */
class DeleteLineCommand : public Command {
private:
    std::unordered_map<std::string, std::string>& working_directory;
    std::string filename;
    int line_number;
    std::string deleted_content;  // Stores the line deleted in execute() for undo

public:
    /**
     * @brief Constructor for DeleteLineCommand.
     * @param wdir Reference to the Repository's working_directory map
     * @param filename Name of the file to edit
     * @param line_number 1-indexed line number to delete
     */
    DeleteLineCommand(std::unordered_map<std::string, std::string>& wdir,
                      const std::string& filename,
                      int line_number);

    /**
     * @brief Remove the line at the specified line number.
     * @throws std::invalid_argument if line_number < 1 or > current_lines
     * @throws std::runtime_error if the file does not exist in working_directory
     */
    void execute() override;

    /**
     * @brief Re-insert the deleted line at its original position.
     */
    void undo() override;
};

/**
 * @class ImportFileCommand
 * @brief Command to import external file content into the working_directory.
 *
 * Inserts or overwrites a file in working_directory. If the file existed before,
 * undo() restores the old content. If it did not exist, undo() removes the key.
 */
class ImportFileCommand : public Command {
private:
    std::unordered_map<std::string, std::string>& working_directory;
    std::string filename;
    std::string content;
    bool existed_before;
    std::string old_content;

public:
    /**
     * @brief Constructor for ImportFileCommand.
     * @param wdir Reference to the Repository's working_directory map
     * @param filename Name of the file to import
     * @param content The new content to import
     *
     * The constructor captures whether the file existed before import and stores
     * the old content if it did, enabling proper undo behavior.
     */
    ImportFileCommand(std::unordered_map<std::string, std::string>& wdir,
                      const std::string& filename,
                      const std::string& content);

    /**
     * @brief Insert or overwrite the file with the imported content.
     */
    void execute() override;

    /**
     * @brief Restore the file to its state before import (or remove it if new).
     */
    void undo() override;
};

/**
 * @class RemoveFileCommand
 * @brief Command to remove a file from the working_directory.
 *
 * Removes a file and stores its content for restoration via undo().
 * Throws if the file does not exist when constructed.
 */
class RemoveFileCommand : public Command {
private:
    std::unordered_map<std::string, std::string>& working_directory;
    std::string filename;
    std::string stored_content;

public:
    /**
     * @brief Constructor for RemoveFileCommand.
     * @param wdir Reference to the Repository's working_directory map
     * @param filename Name of the file to remove
     *
     * @throws std::invalid_argument if filename is not in working_directory
     *
     * The constructor immediately captures the filename and its full content
     * before deletion, ensuring that undo() can fully restore the file.
     */
    RemoveFileCommand(std::unordered_map<std::string, std::string>& wdir,
                      const std::string& filename);

    /**
     * @brief Erase the file from working_directory.
     */
    void execute() override;

    /**
     * @brief Re-insert the file with its stored content.
     */
    void undo() override;
};
