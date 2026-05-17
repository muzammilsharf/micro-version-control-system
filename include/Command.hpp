/*Implement the Command Pattern for file editing to support undo/redo functionality.
Generate Command.hpp with an abstract base class Command having pure virtual execute(Repository& repo) and undo(Repository& repo) methods.
Then, generate Command.cpp implementing three derived classes:
1. AddLineCommand: Adds a line to a specific file in the repository's working_directory.
2. DeleteLineCommand: Removes a line and stores the deleted content for undo.
3. RemoveFileCommand: Erases a file from the working_directory but deeply copies the text content internally so undo() can perfectly restore the file.
Ensure operations modify the strings in the map safely, handling \n delimiters.*/