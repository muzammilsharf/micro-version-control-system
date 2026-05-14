# Micro-VCS: C++ Version Control System Engine

![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Build](https://img.shields.io/badge/Build-CLI-orange.svg)
![Status](https://img.shields.io/badge/Status-Active-success.svg)

Micro-VCS is a high-performance, command-line version control backend engineered entirely in standard C++. 

Rather than relying on external databases or high-level frameworks, this system explores the raw, fundamental data structures that power modern DevOps tools like Git. It implements custom memory management, cryptographic state hashing, and non-linear data serialization to manage repositories and file histories efficiently.

## Core Architectural Features

* **Role-Based Access Control (RBAC):** Utilizes $O(1)$ Hash Maps (`std::unordered_map`) to enforce three strict privilege tiers: Global Admin, Authenticated User (isolated workspaces), and Guest (read-only search).
* **Global Prefix-Tree Search:** Implements a custom **Trie** data structure to aggregate system-wide public repositories, enabling lightning-fast $O(L)$ autocomplete search capabilities.
* **Dual-Stack State Tracking:** Uses a Command Pattern driven by `std::stack` (Undo/Redo) to manage live, volatile file modifications during active sessions without corrupting the save state.
* **DAG-Based Time Travel:** Stores committed histories as an immutable **Directed Acyclic Graph (DAG)**. Unchanged files maintain memory pointers to previous states (preventing duplication), while altered files branch forward.
* **Custom Object Serialization:** Features a robust file I/O engine that parses complex, pointer-based structures (Hash Maps, Trees, Graphs) into delimiter-separated `.txt` files, allowing complete RAM reconstruction upon reboot.

## Project Structure

```text
MicroVCS/
├── CMakeLists.txt              # Build configuration
├── README.md                   # Project documentation
│
├── include/                    # Header files (Declarations)
│   ├── SystemCore.h            # File Serialization (Save/Load) engine
│   ├── User.h                  # RBAC and User struct definitions
│   ├── Trie.h                  # Custom Prefix Tree implementation
│   ├── HistoryTracker.h        # DAG Nodes, Hashes, and Undo/Redo Stacks
│   └── CLI.h                   # Terminal REPL and UI logic
│
├── src/                        # Source files (Implementations)
│   ├── SystemCore.cpp
│   ├── User.cpp
│   ├── Trie.cpp
│   ├── HistoryTracker.cpp
│   ├── CLI.cpp
│   └── main.cpp                # Main system bootloader
│
└── data/                       # Local persistent storage
    ├── users.txt               # Serialized RBAC matrix and dashboards
    ├── repos.txt               # Serialized Trie states
    └── commits.txt             # Serialized DAG history

```

## Build Instructions
This project requires a C++ compiler supporting the C++17 standard (e.g., GCC, Clang) and CMake.

* **Clone the repository:**

```
git clone [https://github.com/yourusername/MicroVCS.git](https://github.com/yourusername/MicroVCS.git)
cd MicroVCS
```

* **Build via CMake:**
```
mkdir build
cd build
cmake ..
make
```

* **Run the engine:**

```
./MicroVCS
```

## CLI Command Reference
Once the REPL (Read-Eval-Print Loop) is active, you can interact with the system using the following syntax:

**Authentication:**

* LOGIN `<username> <password>` - Authenticate session.

* LOGOUT - Terminate active session and serialize data.

**Workspace Management:**

* CREATE_REPO `<name> <public|private>` - Initialize a new repository workspace.

* SEARCH `<prefix>` - Query the global Trie for matching repositories.

* DASHBOARD - Display personal repository metrics and user stats.

**State Tracking (Volatile):**

* WRITE `<filename>` "text" - Add modifications to the active memory buffer.

* UNDO - Revert the last WRITE command via the Undo Stack.

* REDO - Restore an undone action via the Redo Stack.

**Version Control (Persistent):**

- COMMIT "message" - Freeze volatile stacks into an immutable DAG node.

- LOG - Traverse DAG pointers backward to print commit history.

- CHECKOUT `<hash>` - Move the HEAD pointer to instantly restore a previous state.

## Engineering Note
Micro-VCS avoids using std::map (which runs in O(logn) due to Red-Black Trees) for authentication, favoring std::unordered_map to achieve strict O(1) lookups. The use of a Trie for the global search engine intentionally trades memory overhead (node pointers) for prefix-matching speed, as standard Hash Maps cannot perform wildcard or partial string matches efficiently.

<BR> Developed for Data Structures & Algorithms, NUML Spring 2026. </BR>