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