/*Generate src/main.cpp. This is the primary bootloader for the Micro-VCS CLI application.

Include the necessary headers (AuthManager.hpp, RepoManager.hpp, VCSEngine.hpp, PersistenceManager.hpp, CLI.hpp).

In the main() function, instantiate the core managers.

Call PersistenceManager::load_all_state() inside a try-catch block to ensure the system boots even if the flat files are missing or empty.

Instantiate the CLI class, passing references to the managers.

Call CLI.start_repl() to begin the interactive terminal loop.

Ensure the program returns 0 on a clean exit. Keep it strictly console-based; no GUI elements.*/
