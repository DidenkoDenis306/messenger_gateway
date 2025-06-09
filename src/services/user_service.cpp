#include "services/user_service.h"
#include "common/logger.h"
#include <algorithm>
#include <regex>
#include <mutex>

UserService::UserService(int port) : HttpService("UserService", port) {
    create_sample_users();
}

void UserService::setup_routes() {
    auto& server = get_server();

    // User management endpoints
    server.Get("/api/users/profile", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_user(req, res);
    });

    server.Put("/api/users/profile", [this](const httplib::Request& req, httplib::Response& res) {
        handle_update_user(req, res);
    });

    server.Get("/api/users", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_users(req, res);
    });

    server.Get("/api/users/search", [this](const httplib::Request& req, httplib::Response& res) {
        handle_search_users(req, res);
    });

    server.Post("/api/users/status", [this](const httplib::Request& req, httplib::Response& res) {
        handle_set_online_status(req, res);
    });

    LOG_INFO("User Service routes configured");
}

void UserService::handle_get_user(const httplib::Request& req, httplib::Response& res) {
    std::string username;
    if (!validate_auth_token(req, username)) {
        send_error_response(res, 401, "Authentication required");
        return;
    }

    std::lock_guard<std::mutex> lock(users_mutex_);
    auto it = users_.find(username);
    if (it == users_.end()) {
        send_error_response(res, 404, "User not found");
        return;
    }

    json response = user_to_json(it->second);
    send_json_response(res, 200, response);
    LOG_INFO("Profile retrieved for user: " + username);
}

void UserService::handle_update_user(const httplib::Request& req, httplib::Response& res) {
    std::string username;
    if (!validate_auth_token(req, username)) {
        send_error_response(res, 401, "Authentication required");
        return;
    }

    try {
        auto request_data = json::parse(req.body);

        std::lock_guard<std::mutex> lock(users_mutex_);
        auto it = users_.find(username);
        if (it == users_.end()) {
            send_error_response(res, 404, "User not found");
            return;
        }

        // Update user fields
        if (request_data.contains("full_name")) {
            it->second.full_name = request_data["full_name"];
        }
        if (request_data.contains("email")) {
            it->second.email = request_data["email"];
        }

        json response = user_to_json(it->second);
        response["updated"] = true;

        send_json_response(res, 200, response);
        LOG_INFO("Profile updated for user: " + username);

    } catch (const json::exception& e) {
        send_error_response(res, 400, "Invalid JSON format");
        LOG_ERROR("JSON parsing error in user update: " + std::string(e.what()));
    }
}

void UserService::handle_get_users(const httplib::Request& req, httplib::Response& res) {
    std::string username;
    if (!validate_auth_token(req, username)) {
        send_error_response(res, 401, "Authentication required");
        return;
    }

    std::lock_guard<std::mutex> lock(users_mutex_);
    json users_array = json::array();

    for (const auto& [user_id, user] : users_) {
        if (user_id != username) { // Don't include requesting user
            json user_info = {
                {"username", user.username},
                {"full_name", user.full_name},
                {"is_online", user.is_online},
                {"last_seen", user.last_seen}
            };
            users_array.push_back(user_info);
        }
    }

    json response = {
        {"users", users_array},
        {"total", users_array.size()}
    };

    send_json_response(res, 200, response);
    LOG_INFO("Users list retrieved for: " + username);
}

void UserService::handle_search_users(const httplib::Request& req, httplib::Response& res) {
    std::string username;
    if (!validate_auth_token(req, username)) {
        send_error_response(res, 401, "Authentication required");
        return;
    }

    std::string query = req.get_param_value("q");
    if (query.empty()) {
        send_error_response(res, 400, "Search query is required");
        return;
    }

    std::lock_guard<std::mutex> lock(users_mutex_);
    json results = json::array();

    std::transform(query.begin(), query.end(), query.begin(), ::tolower);

    for (const auto& [user_id, user] : users_) {
        if (user_id == username) continue; // Skip self

        std::string lower_username = user.username;
        std::string lower_fullname = user.full_name;
        std::transform(lower_username.begin(), lower_username.end(), lower_username.begin(), ::tolower);
        std::transform(lower_fullname.begin(), lower_fullname.end(), lower_fullname.begin(), ::tolower);

        if (lower_username.find(query) != std::string::npos ||
            lower_fullname.find(query) != std::string::npos) {
            json user_info = {
                {"username", user.username},
                {"full_name", user.full_name},
                {"is_online", user.is_online}
            };
            results.push_back(user_info);
        }
    }

    json response = {
        {"results", results},
        {"query", req.get_param_value("q")},
        {"total", results.size()}
    };

    send_json_response(res, 200, response);
    LOG_INFO("User search performed: " + query + " (" + std::to_string(results.size()) + " results)");
}

void UserService::handle_set_online_status(const httplib::Request& req, httplib::Response& res) {
    std::string username;
    if (!validate_auth_token(req, username)) {
        send_error_response(res, 401, "Authentication required");
        return;
    }

    try {
        auto request_data = json::parse(req.body);

        if (!request_data.contains("is_online")) {
            send_error_response(res, 400, "Online status is required");
            return;
        }

        bool is_online = request_data["is_online"];

        std::lock_guard<std::mutex> lock(users_mutex_);
        auto it = users_.find(username);
        if (it != users_.end()) {
            it->second.is_online = is_online;
            it->second.last_seen = std::time(nullptr);
        }

        json response = {
            {"username", username},
            {"is_online", is_online},
            {"updated", true}
        };

        send_json_response(res, 200, response);
        LOG_INFO("Online status updated for user: " + username + " -> " + (is_online ? "online" : "offline"));

    } catch (const json::exception& e) {
        send_error_response(res, 400, "Invalid JSON format");
        LOG_ERROR("JSON parsing error in status update: " + std::string(e.what()));
    }
}

bool UserService::user_exists(const std::string& username) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    return users_.find(username) != users_.end();
}

json UserService::user_to_json(const User& user) {
    return {
        {"username", user.username},
        {"email", user.email},
        {"full_name", user.full_name},
        {"is_online", user.is_online},
        {"last_seen", user.last_seen},
        {"created_at", user.created_at}
    };
}

bool UserService::validate_auth_token(const httplib::Request& req, std::string& username) {
    std::string auth_header = req.get_header_value("Authorization");

    if (auth_header.empty() || auth_header.substr(0, 7) != "Bearer ") {
        return false;
    }

    std::string token = auth_header.substr(7);

    // Simple token validation (in real app, validate JWT properly)
    if (token.substr(0, 4) == "jwt_") {
        size_t start = 4;
        size_t end = token.find('_', start);
        if (end != std::string::npos) {
            username = token.substr(start, end - start);
            return user_exists(username);
        }
    }

    return false;
}

void UserService::create_sample_users() {
    std::time_t now = std::time(nullptr);

    users_["alice"] = {
        "alice", "alice@example.com", "Alice Johnson",
        true, now, now - 86400
    };

    users_["bob"] = {
        "bob", "bob@example.com", "Bob Smith",
        false, now - 3600, now - 172800
    };

    users_["charlie"] = {
        "charlie", "charlie@example.com", "Charlie Brown",
        true, now, now - 259200
    };

    LOG_INFO("Sample users created");
}