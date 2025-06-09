#include "services/auth_service.h"
#include "common/logger.h"
#include <random>
#include <sstream>
#include <iomanip>

AuthService::AuthService(const int port) : HttpService("AuthService", port) {
}

void AuthService::setup_routes() {
    auto& server = get_server();

    // Authentication endpoints
    server.Post("/api/auth/login", [this](const httplib::Request& req, httplib::Response& res) {
        handle_login(req, res);
    });

    server.Post("/api/auth/register", [this](const httplib::Request& req, httplib::Response& res) {
        handle_register(req, res);
    });

    server.Post("/api/auth/verify", [this](const httplib::Request& req, httplib::Response& res) {
        handle_verify_token(req, res);
    });

    server.Post("/api/auth/refresh", [this](const httplib::Request& req, httplib::Response& res) {
        handle_refresh_token(req, res);
    });

    server.Post("/api/auth/logout", [this](const httplib::Request& req, httplib::Response& res) {
        handle_logout(req, res);
    });

    LOG_INFO("Auth Service routes configured");
}

void AuthService::handle_login(const httplib::Request& req, httplib::Response& res) {
    try {
        auto request_data = json::parse(req.body);

        if (!request_data.contains("username") || !request_data.contains("password")) {
            send_error_response(res, 400, "Username and password are required");
            return;
        }

        const std::string username = request_data["username"];
        const std::string password = request_data["password"];

        LOG_INFO("Login attempt for user: " + username);

        if (validate_credentials(username, password)) {
            const std::string token = generate_jwt_token(username);
            const json response = create_user_response(username, token);
            send_json_response(res, 200, response);
            LOG_INFO("Login successful for user: " + username);
        } else {
            send_error_response(res, 401, "Invalid credentials");
            LOG_WARNING("Login failed for user: " + username);
        }

    } catch (const json::exception& e) {
        send_error_response(res, 400, "Invalid JSON format");
        LOG_ERROR("JSON parsing error in login: " + std::string(e.what()));
    }
}

void AuthService::handle_register(const httplib::Request& req, httplib::Response& res) {
    try {
        auto request_data = json::parse(req.body);

        if (!request_data.contains("username") || !request_data.contains("password") ||
            !request_data.contains("email")) {
            send_error_response(res, 400, "Username, password, and email are required");
            return;
        }

        const std::string username = request_data["username"];
        std::string email = request_data["email"];

        LOG_INFO("Registration attempt for user: " + username);

        // Simple validation (in real app, check if user exists)
        if (username.length() < 3) {
            send_error_response(res, 400, "Username must be at least 3 characters");
            return;
        }

        const std::string token = generate_jwt_token(username);
        json response = create_user_response(username, token);
        response["email"] = email;
        response["created"] = true;

        send_json_response(res, 201, response);
        LOG_INFO("Registration successful for user: " + username);

    } catch (const json::exception& e) {
        send_error_response(res, 400, "Invalid JSON format");
        LOG_ERROR("JSON parsing error in registration: " + std::string(e.what()));
    }
}

void AuthService::handle_verify_token(const httplib::Request& req, httplib::Response& res) {
    const std::string auth_header = req.get_header_value("Authorization");

    if (auth_header.empty() || auth_header.substr(0, 7) != "Bearer ") {
        send_error_response(res, 401, "Missing or invalid Authorization header");
        return;
    }

    std::string token = auth_header.substr(7);

    if (verify_jwt_token(token)) {
        const json response = {
            {"valid", true},
            {"message", "Token is valid"}
        };
        send_json_response(res, 200, response);
        LOG_INFO("Token verification successful");
    } else {
        send_error_response(res, 401, "Invalid or expired token");
        LOG_WARNING("Token verification failed");
    }
}

void AuthService::handle_refresh_token(const httplib::Request& req, httplib::Response& res) {
    try {
        auto request_data = json::parse(req.body);

        if (!request_data.contains("refresh_token")) {
            send_error_response(res, 400, "Refresh token is required");
            return;
        }

        // In real app, verify refresh token
        std::string username = "user"; // Extract from refresh token
        std::string new_token = generate_jwt_token(username);

        json response = {
            {"access_token", new_token},
            {"token_type", "Bearer"},
            {"expires_in", 3600}
        };

        send_json_response(res, 200, response);
        LOG_INFO("Token refresh successful");

    } catch (const json::exception& e) {
        send_error_response(res, 400, "Invalid JSON format");
        LOG_ERROR("JSON parsing error in token refresh: " + std::string(e.what()));
    }
}

void AuthService::handle_logout(const httplib::Request& req, httplib::Response& res) {
    json response = {
        {"message", "Logout successful"}
    };
    send_json_response(res, 200, response);
    LOG_INFO("User logout");
}

bool AuthService::validate_credentials(const std::string& username, const std::string& password) {
    // Simple validation for demo (in real app, check against database)
    return !username.empty() && !password.empty() && password.length() >= 4;
}

std::string AuthService::generate_jwt_token(const std::string& username) {
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

bool AuthService::verify_jwt_token(const std::string& token) {
    // Simple validation for demo (in real app, verify JWT signature)
    return !token.empty() && token.substr(0, 4) == "jwt_";
}

json AuthService::create_user_response(const std::string& username, const std::string& token) {
    return {
        {"username", username},
        {"access_token", token},
        {"token_type", "Bearer"},
        {"expires_in", 3600}
    };
}