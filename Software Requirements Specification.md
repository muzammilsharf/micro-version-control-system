**Software Requirements Specification**

**Micro Version Control System (Micro-VCS)**

**Version:** 1.0  
**Standard:** IEEE 830-1998  
**Language:** C++17  
**Build System:** GNU Make

**1\. Introduction**

**1.1 Purpose**

This document is the complete Software Requirements Specification (SRS) for the Micro Version Control System (Micro-VCS), prepared in conformance with IEEE Std 830-1998. It defines the functional behavior, data structures, persistence mechanisms, and access control logic required to implement and evaluate the system. The intended audience includes the course evaluator, the developer, and any collaborating reviewers.

**1.2 Scope**

Micro-VCS is a C++17 command-line application that emulates core behaviors of a distributed version control system. The product:

-   Authenticates users against a persistent, file-backed hash map and enforces a 3-tier Role-Based Access Control (RBAC) model.
-   Maintains per-user repository reference lists and a global prefix-tree (Trie) for system-wide repository search, backed by a global repository hash map.
-   Tracks uncommitted, in-session file changes using a Command Pattern backed by dual stacks (undo/redo) utilizing smart pointers.
-   Records committed snapshots as immutable nodes in a **Commit Chain**, allowing backward traversal via CHECKOUT <hash>.
-   Serializes all in-memory data structures (hash maps, tries, Tree) into human-readable flat files and reconstructs them on startup, with no external database dependency.
-   The system does not implement networking, remote push/pull, branch merging, or conflict resolution. All operations are local and single-user per active session.

**1.3 Definitions, Acronyms, and Abbreviations**

| **Term** | **Definition** |
| --- | --- |
| RBAC | Role-Based Access Control — a method of restricting operations based on a user's assigned role |
| Commit chain | A singly linked list representing the linear, chronological history of commits, managed via parent hash pointers. |
| Trie | Prefix Tree — a tree data structure for efficient string prefix matching |
| Hash Map | std::unordered_map — provides O(1) average-time key-value lookup |
| Command Pattern | A behavioral design pattern that encapsulates an operation as an object, enabling undo/redo |
| Session | The active runtime context after a successful login, holding the current user's role and workspace |
| Commit Hash | A unique identifier (e.g., SHA-like string or timestamp-seeded integer) assigned to each committed snapshot |
| Serialization | The process of converting in-memory data structures into a flat text format for disk storage |
| Deserialization | The reverse process: reading flat text files and reconstructing in-memory data structures |
| AuthManager | Handles Login, registration, and role based access control |
| RepoManager | Handles CRUD operations on repositories and trie indexing |
| VCSEngine | Handles Undo/Redo stacks, the commit chain, and file states |
| PersistenceManager | Handles Atomic serialization/Deserialization |
| CRUD | Create, Read, Update, Delete. the four fundamental data operations |
| GUEST | Read-only privilege tier |
| USER | Repository-scoped read/write privilege tier |
| ADMIN | Global override privilege tier |

**1.4 References**

-   IEEE Std 830-1998: IEEE Recommended Practice for Software Requirements Specifications
-   Cormen, T. H., et al. — Introduction to Algorithms, 3rd Edition (Tree, Trie definitions)
-   Gamma, E., et al. — Design Patterns: Elements of Reusable Object-Oriented Software (Command Pattern)
-   ISO/IEC 14882:2017 (C++17 Standard)

**1.5 Overview**

Section 2 describes the product context, user classes, and constraints. Section 3 provides the detailed functional requirements. Section 4 specifies the data structures and their relationships. Section 5 defines data flow. Sections 6 and 7 cover interface and non-functional requirements. Sections 8 and 9 cover project structure and Copilot workflow guidelines.

**2\. Overall Description**

**2.1 Product Perspective**

Micro-VCS is a self-contained educational system. It does not integrate with Git, SVN, or any real VCS. It is intended to demonstrate practical application of core Data Structures (Hash Map, Trie, Stack, Tree) within a software engineering context. It operates entirely on the local filesystem using three persistent flat files: users.txt, repos.txt, and commits.txt.

**2.2 Product Functions (Summary)**

-   **F1:** User authentication with O(1) lookup.
-   **F2:** Session management with role-based privilege enforcement.
-   **F3:** Repository creation, listing, and deletion (scope varies by role).
-   **F4:** Global autocomplete repository search via Trie.
-   **F5:** Live file editing with undo/redo via Command Pattern stacks.
-   **F6:** Commit creation storing a Chain node with a snapshot and hash.
-   **F7:** Commit checkout restoring a past file state from a Chain node.
-   **F8:** Serialization of all data structures to disk on commit/exit.
-   **F9:** Deserialization and full state reconstruction on startup.

**2.3 User Classes and Characteristics**

| **Role** | **Description** | **Capabilities** |
| --- | --- | --- |
| **ADMIN** | System administrator with global visibility | Full CRUD on all users and all repositories; can promote/demote users |
| **USER** | Standard authenticated user | Full CRUD on own repositories; read access to public repositories; full undo/redo and commit capability |
| **GUEST** | Unauthenticated or read-only session | Can search and view public repository metadata only; no write operations |

**2.4 Operating Environment**

-   **OS:** Linux / macOS / Windows (with POSIX-compatible terminal or WSL)
-   **Compiler:** g++ with -std=c++17 flag
-   **Build Tool:** GNU Make
-   **Runtime:** No external libraries; standard library only (<unordered\_map>, <vector>, <stack>, <fstream>, <sstream>, <filesystem>, <string>)
-   **Storage:** Local filesystem; data files stored under ./data/

**2.5 Design and Implementation Constraints**

-   **C.1:** No external database, ORM, or third-party library may be used.
-   **C.2:** All data structures must be implemented using C++ STL containers or custom implementations.
-   **C.3:** The system must compile cleanly under g++ -std=c++17 -Wall -Wextra with zero errors.
-   **C.4:** Serialization must produce human-readable, delimiter-separated text files utilizing proper delimiter escaping.
-   **C.5:** The Commit Chain must be traversable backward from HEAD to the root commit.
-   **C.6:** The Trie must support O(L) prefix search where L is the length of the search prefix.
-   **C.7:** No dynamic memory leak shall exist; all polymorphic commands and heap-allocated nodes must be managed via destructors or std::unique\_ptr.

**2.6 Assumptions and Dependencies**

-   The developer assumes a single active session at a time (no concurrency).
-   data/ directory must exist before first run; the system will create it if absent using std::filesystem::create\_directories.
-   Commit hashes are assumed unique; the system will implement a deterministic generation strategy using a combination of a system timestamp and an incrementing integer counter.
-   All text files use UTF-8 encoding and Unix line endings.
-   **Memory Scaling:** Because import\_file pushes the entire imported file contents into an Undo\_Stack command object, the system assumes that RAM usage will scale linearly with the size and frequency of imported files during a single active session.

**3\. Specific Requirements**

**3.1 Functional Requirements**

**3.1.1 Authentication & Session Management**

**REQ-AUTH-01: User Login**

-   **Description:** The system shall authenticate a user given a username and password via the command login <username> <password>. Guest access is handled as an exception, utilizing the command login guest which bypasses password validation.
-   **Input:** Username string, password string.
-   **Processing:** The system performs a lookup in Users\_HashMap (std::unordered\_map<string, User>) using the username as key. If found, it compares the stored password against the input. On success, a Session object is instantiated with the user's ID, role, and a session token.
-   **Output:** A session token and role-confirmation message, or an "Authentication failed" error.
-   **Performance:** Lookup complexity shall be O(1) average case, guaranteed by std::unordered\_map.
-   **Error Handling:** If the username does not exist in the map, or the password does not match, the system shall print an error and return to the login prompt without crashing.

**REQ-AUTH-02: ADMIN Registration (ADMIN)**

-   **Description:** An authenticated ADMIN shall be able to explicitly register new accounts and assign them any system role using the command register <username> <password> <role>.
-   **Input:** New username, password, and role (ADMIN/USER/GUEST).
-   **Processing:** The system validates that the current session holds ADMIN privileges. It then checks for duplicate usernames in the Users\_HashMap. If unique, it creates a new User struct, hashes the password, inserts the object into the hash map, and serializes the change to data/users.txt.
-   **Error Handling:** If a duplicate username is detected, the system shall reject the request with a "Username already exists" error. If a non-admin attempts this command, it shall throw an "Unauthorized Access" error.

**REQ-AUTH-03: USER Registration**

-   **Description:** An unauthenticated user or guest shall be able to register themselves for a standard workspace account using the command register <username> <password>.
-   **Input:** New Username (string), Password (string).
-   **Processing:** Checks for duplicate usernames in the Users\_HashMap. If unique, it creates a new User struct and automatically assigns the USER role. It hashes the password, inserts the user into the hash map, and serializes the state to data/users.txt.
-   **Error Handling:** Duplicate usernames shall be rejected with a descriptive error message preventing account creation.

**REQ-AUTH-04: Session Termination**

-   **Description:** The command logout shall terminate the active session. The command exit shall serialize all data and shut down the application.
-   **Processing on exit:** Triggers the Serialization Engine (REQ-PERSIST-01) before process termination.

**REQ-AUTH-05: Role Enforcement**

-   **Description:** Every command handler shall check the active session's privilege level before executing. Operations exceeding the user's role shall return a "Permission denied" message.

**REQ-AUTH-06: View Users (ADMIN)**

-   **Description:** list\_users shall print a tabular list of all registered users and their assigned roles.
-   **Processing:** Iterates through Users\_HashMap and prints the id and role fields. Operation is strictly restricted to the ADMIN role.
-   **Privilege Matrix:**

| **Command** | **ADMIN** | **USER** | **GUEST** |
| --- | --- | --- | --- |
| help | ✓ | ✓ | ✓ |
| login / logout | ✓ | ✓ | ✓ (Guest Login) |
| register | ✓ | ✓ | ✗ (Must be unauthenticated) |
| list_user | ✓ | ✗ | ✗ |
| create_repo | ✓ | ✓ | ✗ |
| delete_repo | ✓ (Any) | ✓ (Own) | ✗ |
| list_repos | ✓ (All) | ✓ (Own) | ✗ |
| list_public_repos | ✓ | ✓ | ✓ |
| search | ✓ | ✓ | ✓ (Public only) |
| log | ✓ (Any) | ✓ (Own + Public) | ✓ (Public only) |
| status | ✓ (Any) | ✓ (Own) | ✗ |
| view_file | ✓ (Any) | ✓ (Own + Public) | ✓ (Public only) |
| import_file | ✓ (Any) | ✓ (Own) | ✗ |
| remove_file | ✓ (Any) | ✓ (Own) | ✗ |
| add_line / delete_line | ✓ (Any) | ✓ (Own) | ✗ |
| undo / redo | ✓ (Any) | ✓ (Own) | ✗ |
| reset | ✓ (Any) | ✓ (Own) | ✗ |
| commit | ✓ (Any) | ✓ (Own) | ✗ |
| checkout | ✓ (Any) | ✓ (Own) | ✗ |

**3.1.2 Global Usability & Discovery**

**REQ-SYS-01: Dynamic Help Menu**

-   **Description:** The system shall provide a help command that prints a formatted list of all available commands.
-   **Processing:** The system evaluates the active session's Role (ADMIN, USER, or GUEST) and filters the output to display only the commands that the active role is authorized to execute.

**3.1.3 Repository Management**

**REQ-REPO-01: Repository Creation**

-   **Description:** An authenticated USER or ADMIN shall create a repository using create\_repo <name> <public|private>.
-   **Processing:** A new Repository object is instantiated with a unique ID. The repository is inserted into a globally scoped Global\_Repo\_Map (std::unordered\_map<string, Repository>) strictly using the **Repository Name** as the key to ensure O(1) lookups globally. The unique repository ID (string) is appended to the owner's repo\_list (std::vector<string>) inside their User struct. If public, the ID is also inserted into the global Repo\_Trie.
-   **Constraints:** Repository names must be unique per user. Name length shall not exceed 64 characters.

**REQ-REPO-02: Per-User Repository Dashboard**

-   **Description:** The command list\_repos shall display only the repositories owned by the currently authenticated user.
-   **Processing**: Retrieves the active user's repo\_list vector of IDs, iterates over it, and performs an O(1) lookup in Global\_Repo\_Map to display name, visibility, and commit count..
-   **Data Structure Used:** std::vector<std::string> (storing repository names) inside the User struct.

**REQ-REPO-03: Repository Deletion**

-   **Description:** delete\_repo <name> shall remove a repository owned by the active user (USER role) or any repository (ADMIN role).
-   **Processing:** If public, the repository name is removed from the Repo\_Trie using a tombstone approach: traverse to the terminal node of the name, set is\_end\_of\_word = false, and clear the stored repository\_id. Existing structural nodes are left intact to avoid breaking shared prefixes, optimizing deletion to O(L) time.

**REQ-REPO-04: Global Autocomplete Search**

-   **Description:** The command search <prefix> shall return all public repository names in the system that begin with the given prefix.
-   **Processing:** During a prefix search, the Trie returns a list of these names, which are then used as direct keys for O(1) lookups in the Global\_Repo\_Map.
-   **Performance:** O(L + K) where L is the prefix length and K is the number of matching results.
-   **Node Defination:** The Repo\_Trie is keyed by repository name. The terminal node (where is\_end\_of\_word == true) simply stores the corresponding repository\_name as its payload.
-   **Scope:** Only public repositories are indexed in the Trie.

**REQ-REPO-05: Browse Public Repositories**

-   **Description:** The command list\_public\_repos shall allow any user (including GUEST) to view a list of all public repositories in the system.
-   **Processing:** Iterates over the Global\_Repo\_Map. If is\_public == true, it prints the repository name, owner ID, and commit count. This provides the GUEST role with actionable repository names to use in conjunction with view\_file and log.

**3.1.4 Version Tracking & State Management**

**Scope Constraint:** All Undo\_Stack and Redo\_Stack structures are strictly instantiated per-repository (encapsulated within the Repository struct), not globally. This ensures that live edits in one repository cannot leak into or accidentally revert changes in another repository during an active session.

**REQ-VCS-01: File Editing (Add Line)**

-   **Description:** add\_line <repo> <file> <line\_number> <text> shall insert a line of text into the specified file's current in-memory state.
-   **Processing:** An AddLineCommand or DeleteLineCommand object is created. The command is pushed onto the Undo\_Stack as a std::unique\_ptr<Command>. The Redo\_Stack is cleared automatically safely destroying discarded command memory.
-   **Performance:** O(C) time complexity, where C is the total character count of the file. This accounts for the overhead of splitting the flat string by \\n, modifying the vector index, and rejoining the string.
-   **Data Structure Used:** std::stack<std::unique\_ptr<Command>> (located within the target Repository object).

**REQ-VCS-02: File Editing (Delete Line)**

-   **Description:** delete\_line <repo> <file> <line\_number> shall remove a line from the file's current in-memory state.
-   **Processing:** A DeleteLineCommand object is created encapsulating the inverse operation (re-insert the removed line). Pushed onto Undo\_Stack. Redo\_Stack cleared.
-   **Performance:** O(C) time complexity, where C is the total character count of the file. This accounts for the overhead of splitting the flat string by \\n, modifying the vector index, and rejoining the string.

**REQ-VCS-03: Undo**

-   **Description:** undo shall reverse the most recent uncommitted file modification.
-   **Processing:** Pops the top std::unique\_ptr<Command> from Undo\_Stack, calls its undo() method... and pushes ownership to the Redo\_Stack.
-   **Error Handling:** If Undo\_Stack is empty, print "Nothing to undo."

**REQ-VCS-04: Redo**

-   **Description:** redo shall re-apply the most recently undone modification.
-   **Processing:** Pops the top std::unique\_ptr<Command> from Redo\_Stack, calls execute(), and moves ownership back to Undo\_Stack.
-   **Error Handling:** If Redo\_Stack is empty, print "Nothing to redo."

**REQ-VCS-05: Commit**

-   **Description:** commit <repo> <message> shall take a snapshot of all staged file states and create a permanent Chain node.
-   **Processing:**

A new CommitNode is created. The system performs a complete, all-or-nothing deep copy of the **entire** working\_directory hash map into the new node's file\_snapshot. There is no partial file staging; every file currently present in the map is permanently recorded in the snapshot. If this is the first commit in the repository, the parent\_hash is set to an empty string "" to signify the terminus of the chain.

**REQ-VCS-06: Checkout**

-   **Description:** checkout <repo> <commit\_hash> shall restore the file state associated with the specified commit.
-   **Processing:** Performs an O(1) lookup in the repository's commit\_history hash map (std::unordered\_map<std::string, CommitNode>) using the commit\_hash as the key. If found, the repository's in-memory working\_directory map is replaced with the file\_snapshot stored in that node.
-   **State Resolution (Detached HEAD):** The active session object tracks a boolean is\_detached flag. The CLI shall treat the string "HEAD" as a reserved keyword; typing checkout HEAD clears the flag and returns the user to the most recent commit state.
-   **Error Handling:** If the hash is not found in the Chain, print "Commit not found."

**REQ-VCS-07: View Commit History**

-   **Description:** log <repo> shall print all commit hashes, messages, and timestamps in reverse chronological order (newest first).
-   **Processing:** Starting from the HEAD pointer of the Commit Chain, the system traverses the single parent pointers backward to the root commit, printing each node's metadata along the way.

**REQ-VCS-08: File Importing**

-   **Description:** import\_file <repo> <file\_path> shall read an external text file from the local filesystem and stage its entire contents into the repository's in-memory working directory.
-   **Processing:** The system opens the file via std::ifstream, reads the contents into a single std::string, and assigns it to the working\_directory hash map using the filename as the key. The operation is wrapped in an ImportFileCommand as a std::unique\_ptr<Command> and pushed onto the Undo\_Stack.
-   **File Size Limit:** The system must utilize std::filesystem::file\_size prior to reading. If the file exceeds 1MB (1,048,576 bytes), the operation aborts with a "File exceeds maximum size limit" error to protect heap memory.
-   **Security Constraint***:* To prevent path traversal attacks (e.g., ../../), the file\_path must be validated to ensure it only reads from the intended local workspace directory.
-   **Error Handling:** If the file path is invalid or the file cannot be opened, print "Error: Cannot read file."

**REQ-VCS-09: Workspace Status**

-   **Description:** status <repo> shall display all currently staged files in the uncommitted working directory.
-   **Processing:** The system locates the repository and iterates over the keys of the working\_directory hash map (std::unordered\_map<std::string, std::string>). It prints each filename (key) along with a calculated line count (derived by counting \\n characters in the mapped value).
-   **Performance:** O(F + C) where F is the number of files and C is the total number of characters to count newlines.

**REQ-VCS-10: View File Contents**

-   **Description:** view\_file <repo> <filename> shall output the current in-memory text of a staged file, prefixed with line numbers.
-   **Processing:** Performs an O(1) lookup in the working\_directory hash map using the filename as the key. The resulting string is split by the \\n delimiter and printed to standard output sequentially with an incrementing integer prefix (e.g., 1: ..., 2: ...).
-   **Error Handling**: If the filename does not exist in the working directory, print "File not found in staging area."

**REQ-VCS-11: Remove/Unstage File**

-   **Description:** remove\_file <repo> <filename> shall delete a file from the repository's active staging area.
-   **Processing:** The system performs an O(1) .erase() operation on the map. The RemoveFileCommand must store *both* the filename and a deep copy of the file's text content, so the file can be fully restored upon execution of undo.
-   **REQ-VCS-12: Reset Workspace (The Panic Button)**
-   **Description:** reset <repo> shall instantly clear all uncommitted changes, providing a safe escape hatch from the "Dirty State" block (REQ-EDGE-01) without requiring repetitive manual undos.
-   **Processing:** The system forcefully clears both the Undo\_Stack and Redo\_Stack. It then overwrites the working\_directory hash map with a fresh deep copy of the file\_snapshot from the HEAD CommitNode.

**3.1.5 Persistence & Serialization**

**REQ-PERSIST-01: Serialization on Exit/Commit**

-   **Description:** On commit and application exit, PersistenceManager::save\_all\_state() shall serialize the complete state of all in-memory data structures to disk via atomic file writes.
-   **Files Written:**

| **File** | **Contents** |
| --- | --- |
| data/users.txt | All User entries from Users_HashMap |
| data/repos.txt | All Repository entries, linked to their owner by user ID |
| data/commits.txt | All Chain commit nodes, with parent hash references instead of raw pointers |

-   **Format:** Each record is on its own line. Fields within a record are separated by a | delimiter. Multi-line file content within a commit snapshot is encoded using a \\n escape sequence.
-   **Escaping Strategy:** Multi-line file content is encoded using a \\n sequence. Any instances of the literal | character within user input or file content MUST be encoded as \\| prior to writing to disk to prevent deserialization failure.

**REQ-PERSIST-02: Deserialization on Startup**

-   **Description:** On startup, SystemCore::initialize() shall parse all three data files and reconstruct the in-memory state.
-   **Processing Steps:**
    1.  Parse data/users.txt line-by-line; reconstruct each User struct and insert into Users\_HashMap.
    2.  Parse data/repos.txt (handling the \\| escape sequence); populate Global\_Repo\_Map, push ID to the owner's repo\_list, and index in Repo\_Trie if public.
-   Parse data/commits.txt; reconstruct each CommitNode inside the Chain

**Error Handling:** If any data file is missing or malformed (e.g., wrong field count), the system shall print a warning and initialize with an empty state rather than crashing.

-   **3.2 Constraints Summary**

| **ID** | **Constraint** |
| --- | --- |
| CON-01 | No external libraries. STL only. |
| CON-02 | Compile with g++ -std=c++17 -Wall -Wextra without errors or warnings. |
| CON-03 | All heap allocations for Commit Chain nodes must be freed (use destructors or smart pointers). |
| CON-04 | Serialized files must be human-readable (no binary formats). |
| CON-05 | std::filesystem may be used (C++17) for directory creation. |
| CON-06 | The codebase shall utilize C++ standard exceptions (std::runtime_error, std::invalid_argument) for all core business logic failures (e.g., hash not found, invalid permissions). The CLI layer is strictly responsible for catching these exceptions within a try-catch block and printing a user-friendly error message to standard output, preventing unexpected process termination. |

**3.3 Operational Edge Cases & Data Integrity**

**REQ-EDGE-01: Dirty State Prevention**

-   **Description:** The system shall protect uncommitted work by preventing destructive navigational operations while the working directory is "dirty" (i.e., contains uncommitted changes).
-   **Processing:** Before executing checkout <hash>, the system compares the current working\_directory hash map against the file\_snapshot map of the HEAD commit. If the maps are not identical (size mismatch or content mismatch), the state is considered dirty and the operation is forcefully aborted. *Zero-Commit Exception:* If head\_commit\_hash is empty, the system treats the working directory as clean and bypasses the dirty-check.
-   **Error Handling:** The system shall output the error: "Working directory not clean. Please commit or undo changes before checking out."

**REQ-EDGE-02: Serialization Delimiter Escaping**

-   **Description:** The system must accurately differentiate between structural data delimiters and standard user input during the serialization process to prevent parsing failures.
-   **Processing:**
    -   Prior to parsing, the system scans the data/ directory. If any .tmp files (e.g., commits.txt.tmp) are found, they are permanently deleted to ensure no corrupted partial saves interfere with the current session.
-   Prior to serialization, the system must first escape any backslashes \\ by replacing them with \\\\. Then, it must replace any literal | delimiters within user strings with \\|. During deserialization, this order is strictly reversed
-   **Deserialization:** During startup, the parser must read \\| as a literal | character and must not treat it as a field boundary.

**REQ-EDGE-03: Corrupt Data Recovery & Fault Tolerance**

-   **Description:** Because flat files are highly susceptible to manual corruption (e.g., a user manually editing commits.txt), the deserialization engine (PersistenceManager::load\_all\_state()) must be completely fault-tolerant.
-   **Processing:** While parsing data files line-by-line, the system shall validate the structural integrity of each record (e.g., verifying the correct number of | delimiters per line and ensuring parent hashes actually exist in the Chain).
-   **Error Handling:** If a malformed line or unresolvable pointer is detected:
    1.  The system shall safely discard the corrupt record.
    2.  Log a non-fatal warning to standard output (e.g., WARNING: Skipping corrupted commit record at line 42.).
    3.  Continue parsing the remainder of the file to salvage all valid data.
    4.  Under no circumstances shall a malformed data file result in a segmentation fault or an unhandled C++ exception.

**REQ-EDGE-04: Standard Input Validations**

-   **Line Editing Bounds:** If add\_line or delete\_line receives a line number < 1 or > current lines, abort with "Error: Line number out of range."
-   **Empty Commits:** If commit is executed while the working directory is empty, abort with "Error: Working directory is empty."
-   **Empty Searches:** If search is executed with no prefix, abort with "Error: Please provide a search term."
-   **Private Repo Masking:** To prevent enumeration attacks, accessing a private repository you do not own will return the exact same "Repository not found" message as a non-existent repository.
-   **Dirty Logout:** Typing logout or exit with uncommitted changes will prompt the user with "Uncommitted changes will be lost. Proceed? \[y/N\]" before discarding session state.

**4\. Non-Functional Requirements**

**4.1 Performance**

-   **NFR-PERF-01:** Authentication lookup shall complete in O(1) average time.
-   **NFR-PERF-02:** Trie autocomplete shall complete in O(L + K) time.
-   **NFR-PERF-03:** Undo and Redo operations shall complete in O(1) time (stack pop/push).
-   **NFR-PERF-04:** Serialization of up to 1,000 users, 5,000 repos, and 10,000 commits shall complete within 3 seconds on a standard development machine *(Assumption: average repository size is < 50KB, allowing for full-state snapshot copying)*.
-   **NFR-PERF-05:** Traversing the Commit Chain for history logging shall complete in O(N) time, where N is the number of commits from the current HEAD to the root.

**4.2 Reliability**

-   **NFR-REL-01:** The application shall not crash on malformed or missing data files; it shall initialize to an empty state and warn the user.
-   **NFR-REL-02:** All raw pointer allocations (Commit Chain nodes nodes, Trie nodes) shall be freed before process exit to ensure no memory leaks.
-   **NFR-REL-03:** An interrupted session (SIGINT / Ctrl+C) shall not corrupt existing data files.
    -   **Implementation Strategy (Atomic Writes):** The PersistenceManager shall strictly use atomic file writing. Data must first be serialized to a temporary file (e.g., commits.txt.tmp). Only after the stream is successfully flushed and closed will the system invoke std::filesystem::rename() to overwrite the original file, ensuring data integrity at the OS level.

**4.3 Maintainability**

-   **NFR-MAIN-01:** Each class shall reside in its own header/source file pair.
-   **NFR-MAIN-02:** All public methods shall have a Doxygen-style comment.
-   **NFR-MAIN-03:** Magic numbers shall be replaced with named constants or enums.

**4.4 Portability**

-   **NFR-PORT-01:** The system shall compile without modification on Linux and macOS.
-   **NFR-PORT-02:** No platform-specific headers (e.g., <windows.h>) shall be used.

**Project Structure:**

MicroVCS/

│

├── Makefile # Build instructions for g++ -std=c++17

├── README.md # Project documentation and execution instructions

│

├── include/ # Header files (Declarations)

│ ├── Types.hpp # Data structures (User, Repository, CommitNode, Role)

│ ├── AuthManager.hpp # User hash map, RBAC, and deterministic hashing logic

│ ├── RepoManager.hpp # Global repository map and Trie indexing

│ ├── VCSEngine.hpp # Commit Chain manipulation, checkout, and file states

│ ├── PersistenceManager.hpp # Atomic File Serialization/Deserialization

│ ├── Command.hpp # Command Pattern interfaces (Undo/Redo logic)

│ ├── RepoTrie.hpp # Custom Prefix Tree implementation for fast search

│ └── CLI.hpp # Terminal REPL, exception catching, and UI routing

│

├── src/ # Source files (Implementations)

│ ├── main.cpp # Main system bootloader (instantiates managers & CLI)

│ ├── AuthManager.cpp # Login, registration, and user CRUD logic

│ ├── RepoManager.cpp # Repository CRUD and Trie traversal logic

│ ├── VCSEngine.cpp # History traversal, detached HEAD, and snapshot logic

│ ├── PersistenceManager.cpp # File I/O, delimiter escaping, and corrupt line skipping

│ ├── Command.cpp # Implementations for AddLine, DeleteLine, Import commands

│ ├── RepoTrie.cpp # Trie node insertion and tombstone deletion logic

│ └── CLI.cpp # Terminal string parsing and routing

├── tests/ # Custom Testing Harness (NO EXTERNAL LIBRARIES)

│ ├── test\_auth.cpp # Tests for djb2 hashing and RBAC logic

│ ├── test\_trie.cpp # Tests for O(1) insertions and prefix search

│ ├── test\_vcs.cpp # Tests for Command Pattern undo/redo stacks

│ └── test\_persistence.cpp # Tests for atomic writes and corrupt file handling │

└── data/ # Local persistent storage (Auto-generated at runtime)

├── users.txt # Serialized users, roles, and hashed passwords

├── repos.txt # Serialized repository metadata

└── commits.txt # Serialized Commit Chain history and file snapshots

**Class Diagram:**

![](./Software%20Requirements%20Specification_images/image-001.png)

Figure 1: Class Diagram

**Use-Case Diagram:**

**![](./Software%20Requirements%20Specification_images/image-002.png)**

Figure 2: Use-Case Diagram

**Data Flow Diagram:**

![](./Software%20Requirements%20Specification_images/image-003.png)

Figure 3: Data Flow diagram

**Sequence Diagram:**

![](./Software%20Requirements%20Specification_images/image-004.png)

Figure 4: Sequence Diagram

**System Architecture:**

![](./Software%20Requirements%20Specification_images/image-005.png)

Figure 5: System Architecture

**9\. Testing & Validation Strategy**

Because CON-01 prohibits external testing frameworks, the system shall implement a custom, lightweight testing harness utilizing standard C++ <cassert> macros and isolated test runner executables.

**9.1 Unit Testing**

-   **Data Structures:** The Repo\_Trie and Global\_Repo\_Map shall be tested with hardcoded mock data to verify O(1) insertions and accurate $O(L)$ prefix matching.
-   **Command Pattern:** AddLineCommand and DeleteLineCommand shall be instantiated manually and their execute() and undo() methods tested against a mock Repository object to ensure the string vector manipulation is accurate.

**9.2 Integration Testing**

-   **The Core Loop:** A scripted test sequence shall programmatically simulate a user workflow: login -> create\_repo -> import\_file -> commit -> checkout. The final working directory state must match a predefined expected string.

**9.3 Persistence & Edge Case Testing**

-   **I/O Integrity:** The PersistenceManager shall be tested by generating a mock state, calling save\_all\_state(), clearing RAM, and calling load\_all\_state(). The pre-save and post-load map sizes must be identical.
-   **Fault Tolerance:** A deliberate corruption test will inject a malformed line into commits.txt prior to boot. The system passes if it successfully parses the valid lines and drops the corrupt line without crashing.