# Micro-VCS — A Terminal-Based Version Control System

![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Build](https://img.shields.io/badge/Build-CLI-orange.svg)
![Status](https://img.shields.io/badge/Status-Active-success.svg)

> A fully functional mini version control system built from scratch in C++17, demonstrating advanced data structure design, RBAC, persistent storage, and the Command Pattern.

---

## Overview

Micro-VCS is a command-line application that replicates core Git-like functionality — committing snapshots, checking out history, undoing/redoing edits, and managing repositories with role-based access control. Every feature is backed by a deliberate data structure choice, not just standard library calls.

Built as a Data Structures course project, the system handles multi-user sessions, atomic file persistence, detached HEAD states, and prefix-based repository search — all from scratch with no external libraries.

---

## Technical Highlights

| Area | Implementation |
|---|---|
| Commit History | Singly linked list via `parent_hash` strings inside `CommitNode` |
| O(1) Commit Lookup | `unordered_map<string, unique_ptr<CommitNode>>` per repository |
| Repo Search | Custom hand-written Prefix Trie with tombstone deletion |
| Undo / Redo | `stack<unique_ptr<Command>>` per repository (Command Pattern) |
| User & Repo Index | `unordered_map` with O(1) average-time access |
| Persistence | Pipe-delimited flat files with atomic writes via temp-file rename |
| Memory Safety | `unique_ptr` throughout — zero raw owning pointers |

---

## Data Structures & Design Decisions

### Commit Chain — Singly Linked List
Each `CommitNode` stores a `parent_hash` string. Traversing history means walking this chain backward from HEAD. A parallel `unordered_map<hash, unique_ptr<CommitNode>>` provides O(1) checkout without invalidating pointers on rehash.

### Custom Prefix Trie
Built from scratch with `unordered_map<char, TrieNode*>` children. Supports O(L) insert, O(L) tombstone delete (preserves shared prefixes), and O(L+K) prefix search where K is the result count. Used exclusively for public repository discovery.

### Command Pattern with Undo/Redo
Every file edit (`add_line`, `delete_line`, `import_file`, `remove_file`) is encapsulated as a `unique_ptr<Command>` pushed onto a per-repository undo stack. Undo pops and reverses; redo re-executes. Stacks are scoped per repository, not global.

### RBAC — Role-Based Access Control
Three roles: `ADMIN > USER > GUEST`. Every command handler enforces the minimum required role. Private repository access returns the same error as "not found" — preventing enumeration attacks.

---

## Architecture

```
CLI (REPL)
│
├── AuthManager      — User registration, login, djb2 password hashing
├── RepoManager      — Repository CRUD, owns Global_Repo_Map + RepoTrie
├── VCSEngine        — Commits, checkout, undo/redo, detached HEAD
└── PersistenceManager — Atomic serialization, escape handling, startup recovery
```

All business logic lives in manager classes. The CLI layer only does I/O and exception catching — no logic leaks into handlers.

---

## Features

- **Multi-user sessions** with ADMIN / USER / GUEST roles
- **Repository management** — create, delete, list, search by prefix
- **File editing** — add/delete lines with full undo/redo support
- **Import external files** with path traversal protection and 1MB size limit
- **Commit snapshots** — full working directory captured per commit
- **Checkout** — restore any historical commit, detached HEAD state
- **Persistent storage** — all state survives process restarts
- **Atomic writes** — SIGINT-safe via temp file + rename strategy
- **Fault tolerance** — corrupted data file lines are skipped with warnings

---

## Build & Run

**Requirements:** g++ with C++17 support

```bash
# Build
g++ -std=c++17 -Iinclude src/*.cpp -o bin/microvcs -mconsole

# Run
bin/microvcs
```

---

## Usage Example

```
> register alice pass123
> login alice pass123
> create_repo myproject public
> add_line myproject main.cpp 1 "#include <iostream>"
> add_line myproject main.cpp 2 "int main() { return 0; }"
> status myproject
> commit myproject "initial commit"
> add_line myproject main.cpp 3 "// new line"
> undo myproject
> checkout myproject <hash>
> view_file myproject main.cpp
> checkout myproject HEAD
> exit
```

---

## Project Structure

```
micro-version-control-system/
├── include/
│   ├── Types.hpp             # All structs: User, Repository, CommitNode, Session
│   ├── Command.hpp           # Abstract Command + 4 concrete commands
│   ├── RepoTrie.hpp          # Custom Trie implementation
│   ├── AuthManager.hpp       # Authentication + RBAC
│   ├── RepoManager.hpp       # Repository CRUD + Trie integration
│   ├── VCSEngine.hpp         # VCS operations
│   ├── PersistenceManager.hpp# File I/O + serialization
│   └── CLI.hpp               # REPL interface
├── src/
│   ├── Types.cpp
│   ├── Command.cpp
│   ├── RepoTrie.cpp
│   ├── AuthManager.cpp
│   ├── RepoManager.cpp
│   ├── VCSEngine.cpp
│   ├── PersistenceManager.cpp
│   ├── CLI.cpp
│   └── main.cpp
├── data/                     # Auto-generated at runtime
│   ├── users.txt
│   ├── repos.txt
│   └── commits.txt
└── bin/
    └── microvcs
```

---

## Tech Stack

- **Language:** C++17
- **Libraries:** STL only (no external dependencies)
- **Build:** g++ / MinGW
- **Storage:** Custom pipe-delimited flat file format with backslash escaping

---

## What I Learned

- Designing data structures for real constraints, not just textbook examples
- Why `unique_ptr` ownership semantics matter in complex class hierarchies
- Iterator invalidation in `unordered_map` after insertions
- Atomic file writes and fault-tolerant deserialization
- Separating concerns across manager classes vs letting a god-class own everything
- Working with AI coding agents effectively — prompting with exact types and signatures, verifying generated code against requirements, and knowing when not to trust the output