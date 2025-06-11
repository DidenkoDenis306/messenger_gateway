#include "common/auth_middleware.h"
#include "common/logger.h"
#include <random>
#include <sstream>
#include <iomanip>

AuthMiddleware::AuthResult AuthMiddleware::validate_token(const httplib::Request& req) {
    AuthResult result;
    result.is_valid = false;

    std::string auth_header = req.get_header_value("Authorization");

    if (auth_header.empty() || auth_header.substr(0, 7) != "Bearer ") {
        result.error_message = "Missing or invalid Authorization header";
        return result;
    }

    std::string token = auth_header.substr(7);

    if (!verify_jwt_token(token)) {
        result.error_message = "Invalid or expired token";
        return result;
    }

    result.username = extract_username_from_token(token);
    if (result.username.empty()) {
        result.error_message = "Could not extract username from token";
        return result;
    }

    result.is_valid = true;
    return result;
}

std::string AuthMiddleware::generate_jwt_token(const std::string& username) {
    // Simple token generation for demo (in real app, use proper JWT library)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream token;
    token << "jwt_" << username << "_";

    for (int i = 0; i < 32; ++i) {
        token << std::hex << dis(gen);
    }

    return token.str();
}

bool AuthMiddleware::verify_jwt_token(const std::string& token) {
    // Simple validation for demo (in real app, verify JWT signature)
    return !token.empty() && token.substr(0, 4) == "jwt_";
}

std::string AuthMiddleware::extract_username_from_token(const std::string& token) {
    if (token.substr(0, 4) == "jwt_") {
        size_t start = 4;
        size_t end = token.find('_', start);
        if (end != std::string::npos) {
            return token.substr(start, end - start);
        }
    }
    return "";
}

bool AuthMiddleware::validate_credentials(const std::string& username, const std::string& password) {
    // Simple validation for demo (in real app, check against database)
    return !username.empty() && !password.empty() && password.length() >= 4;
}