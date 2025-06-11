#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

class AuthMiddleware {
public:
    struct AuthResult {
        bool is_valid;
        std::string username;
        std::string error_message;
    };

    static AuthResult validate_token(const httplib::Request& req);
    static std::string generate_jwt_token(const std::string& username);
    static bool verify_jwt_token(const std::string& token);
    static std::string extract_username_from_token(const std::string& token);

private:
    static bool validate_credentials(const std::string& username, const std::string& password);
};