/*Implement the RepoManager class.
It must manage Global_Repo_Map (std::unordered_map<string, Repository>) and an instance of RepoTrie.
Implement create_repo(name, is_public): ensure names are unique (<64 chars), insert into the global map using the name as the key, append the ID to the active user's repo list, and if public, insert into the Trie .
Implement search_public(prefix) which queries the RepoTrie and returns a list of matching repository names.*/