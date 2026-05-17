#include "CLI.hpp"
#include <iostream>

int main() {
    try {
        CLI cli;
        cli.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}