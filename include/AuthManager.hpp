/*
Implement the AuthManager class.
It must manage a Users_HashMap (std::unordered_map<string, User>).
Implement login(username, password) which checks the map in O(1) time and returns a Session object. Include a bypass for login guest.
Implement register_user(username, password, role) which enforces duplicate username checks. Include basic deterministic hashing for passwords (e.g., std::hash or djb2)
Throw std::runtime_error or std::invalid_argument for failures (e.g., 'Username already exists').
*/