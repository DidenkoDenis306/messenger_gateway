#include "handlers/auth_handlers.h"
#include "common/auth_middleware.h"
#include "common/request_validator.h"
#include "common/logger.h"

void AuthHandlers::handle_login(const httplib::Request& req, httplib::Response& res) {
    auto validation = RequestValidator::validate_json_body(req, {"username", "password"});
    if (!validation.is_valid) {
        send_error_response(res, 400, validation.error_message);
        return;
    }

    const std::string username = validation.data["username"];
    const std::string password = validation.data["password"];

    LOG_INFO("Login attempt for user: " + username);

    if (validate_credentials(username, password)) {
        const std::string token = AuthMiddleware::generate_jwt_token(username);
        const json response = create_user_response(username, token);
        send_json_response(res, 200, response);
        LOG_INFO("Login successful for user: " + username);
    } else {
        send_error_response(res, 401, "Invalid credentials");
        LOG_WARNING("Login failed for user: " + username);
    }
}

void AuthHandlers::handle_register(const httplib::Request& req, httplib::Response& res) {
    auto validation = RequestValidator::validate_json_body(req, {"username", "password", "email"});
    if (!validation.is_valid) {
        send_error_response(res, 400, validation.error_message);
        return;
    }

    const std::string username = validation.data["username"];
    const std::string password = validation.data["password"];
    const std::string email = validation.data["email"];

    LOG_INFO("Registration attempt for user: " + username);

    // Validate username
    if (!RequestValidator::is_valid_username(username)) {
        send_error_response(res, 400, "Username must be 3-50 characters and contain only letters, numbers, and underscores");
        return;
    }

    // Validate email
    if (!RequestValidator::is_valid_email(email)) {
        send_error_response(res, 400, "Invalid email format");
        return;
    }

    // Validate password
    if (password.length() < 4) {
        send_error_response(res, 400, "Password must be at least 4 characters");
        return;
    }

    const std::string token = AuthMiddleware::generate_jwt_token(username);
    json response = create_user_response(username, token);
    response["email"] = email;
    response["created"] = true;

    send_json_response(res, 201, response);
    LOG_INFO("Registration successful for user: " + username);
}

void AuthHandlers::handle_verify_token(const httplib::Request& req, httplib::Response& res) {
    auto auth_result = AuthMiddleware::validate_token(req);

    if (auth_result.is_valid) {
        const json response = {
            {"valid", true},
            {"username", auth_result.username},
            {"message", "Token is valid"}
        };
        send_json_response(res, 200, response);
        LOG_INFO("Token verification successful for user: " + auth_result.username);
    } else {
        send_error_response(res, 401, auth_result.error_message);
        LOG_WARNING("Token verification failed: " + auth_result.error_message);
    }
}

void AuthHandlers::handle_refresh_token(const httplib::Request& req, httplib::Response& res) {
    auto validation = RequestValidator::validate_json_body(req, {"refresh_token"});
    if (!validation.is_valid) {
        send_error_response(res, 400, validation.error_message);
        return;
    }

    // In real app, verify refresh token
    std::string username = "user"; // Extract from refresh token
    std::string new_token = AuthMiddleware::generate_jwt_token(username);

    json response = {
        {"access_token", new_token},
        {"token_type", "Bearer"},
        {"expires_in", 3600}
    };

    send_json_response(res, 200, response);
    LOG_INFO("Token refresh successful");
}

void AuthHandlers::handle_logout(const httplib::Request& req, httplib::Response& res) {
    json response = {
        {"message", "Logout successful"}
    };
    send_json_response(res, 200, response);
    LOG_INFO("User logout");
}

bool AuthHandlers::validate_credentials(const std::string& username, const std::string& password) {
    // Simple validation for demo (in real app, check against database)
    return !username.empty() && !password.empty() && password.length() >= 4;
}

json AuthHandlers::create_user_response(const std::string& username, const std::string& token) {
    return {
        {"username", username},
        {"access_token", token},
        {"token_type", "Bearer"},
        {"expires_in", 3600}
    };
}

void AuthHandlers::send_json_response(httplib::Response& res, int status, const json& data) {
    res.status = status;
    res.set_content(data.dump(2), "application/json");
}

void AuthHandlers::send_error_response(httplib::Response& res, int status, const std::string& message) {
    const json error_data = {
        {"error", true},
        {"message", message},
        {"status", status}
    };
    send_json_response(res, status, error_data);
}