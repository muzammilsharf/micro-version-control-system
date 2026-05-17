#include "Types.hpp"
#include <stdexcept>

/**
 * @brief Convert a Role enum value to its string representation.
 * 
 * @param r The Role enum value.
 * @return String representation: "ADMIN", "USER", or "GUEST".
 */
std::string role_to_string(Role r) {
    switch (r) {
        case Role::ADMIN:
            return "ADMIN";
        case Role::USER:
            return "USER";
        case Role::GUEST:
            return "GUEST";
        default:
            throw std::invalid_argument("Unknown role value.");
    }
}

/**
 * @brief Convert a string to its corresponding Role enum value.
 * 
 * @param s The string representation: "ADMIN", "USER", or "GUEST".
 * @return The corresponding Role enum value.
 * @throws std::invalid_argument if the string does not match any role.
 */
Role string_to_role(const std::string& s) {
    if (s == "ADMIN") {
        return Role::ADMIN;
    } else if (s == "USER") {
        return Role::USER;
    } else if (s == "GUEST") {
        return Role::GUEST;
    } else {
        throw std::invalid_argument("Unknown role string: " + s);
    }
}
