/*Implement the CLI class and main.cpp bootloader.
main.cpp should instantiate the Managers and call PersistenceManager::load_all_state(), then start the CLI::start_repl() loop.
The CLI must parse space-separated string tokens and route them to the correct Manager methods.
Crucially, wrap all execution logic in a try-catch block to catch std::runtime_error and std::invalid_argument exceptions thrown by the managers, printing them cleanly to standard output without crashing the REPL. Implement Role-Based Access Control checks before routing commands.*/