#include "services/websocket_service.h"
#include "common/logger.h"
#include <random>
#include <sstream>
#include <iomanip>

WebSocketService::WebSocketService(int port)
    : HttpService("WebSocketService", port),
      should_cleanup_(false),
      connection_counter_(1) {
}

void WebSocketService::setup_routes() {
    auto& server = get_server();

    LOG_INFO("Setting up WebSocket routes...");

    // WebSocket management endpoints
    server.Get("/api/websocket/stats", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_stats(req, res);
    });

    server.Get("/api/websocket/online", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_online_users(req, res);
    });

    server.Post("/api/websocket/connect", [this](const httplib::Request& req, httplib::Response& res) {
        handle_connect_user(req, res);
    });

    server.Post("/api/websocket/send", [this](const httplib::Request& req, httplib::Response& res) {
        handle_send_message(req, res);
    });

    server.Post("/api/websocket/broadcast", [this](const httplib::Request& req, httplib::Response& res) {
        handle_broadcast_message(req, res);
    });

    server.Delete("/api/websocket/disconnect", [this](const httplib::Request& req, httplib::Response& res) {
        handle_disconnect_user(req, res);
    });

    LOG_INFO("WebSocket Service routes configured successfully");
}

void WebSocketService::handle_connect_user(const httplib::Request& req, httplib::Response& res) {
    std::string user_id;

    // Попробуем получить user_id из разных источников

    // 1. Из URL параметров (query string)
    user_id = req.get_param_value("user_id");

    // 2. Если не найден и есть JSON body, попробуем оттуда
    if (user_id.empty() && !req.body.empty()) {
        try {
            auto json_body = json::parse(req.body);
            if (json_body.contains("user_id")) {
                user_id = json_body["user_id"];
            }
        } catch (const json::exception& e) {
            LOG_WARNING("Failed to parse JSON body: " + std::string(e.what()));
        }
    }

    // 3. Попробуем из заголовка авторизации (извлечь username)
    if (user_id.empty()) {
        std::string username;
        if (validate_auth_token(req, username)) {
            user_id = username;
        }
    }

    if (user_id.empty()) {
        send_error_response(res, 400, "user_id parameter is required (in URL params, JSON body, or extracted from auth token)");
        return;
    }

    LOG_INFO("User connecting: " + user_id);

    std::string connection_id = generate_connection_id();
    add_connection(user_id, connection_id);

    // Notify other users about new user online
    notify_user_status_change(user_id, true);

    json response = {
        {"connection_id", connection_id},
        {"user_id", user_id},
        {"connected", true},
        {"timestamp", std::time(nullptr)}
    };

    send_json_response(res, 200, response);
    LOG_INFO("User connected: " + user_id + " (connection: " + connection_id + ")");
}


void WebSocketService::handle_get_stats(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    json response = {
        {"total_connections", connections_.size()},
        {"active_users", user_connections_.size()},
        {"timestamp", std::time(nullptr)}
    };

    send_json_response(res, 200, response);
    LOG_INFO("WebSocket stats retrieved");
}

void WebSocketService::handle_get_online_users(const httplib::Request& req, httplib::Response& res) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    json users_array = json::array();
    for (const auto& [user_id, connection_ids] : user_connections_) {
        if (!connection_ids.empty()) {
            users_array.push_back(user_id);
        }
    }

    json response = {
        {"users", users_array},
        {"total", users_array.size()}
    };

    send_json_response(res, 200, response);
    LOG_INFO("Online users list retrieved (" + std::to_string(users_array.size()) + " users)");
}

void WebSocketService::handle_send_message(const httplib::Request& req, httplib::Response& res) {
    std::string username;
    if (!validate_auth_token(req, username)) {
        send_error_response(res, 401, "Authentication required");
        return;
    }

    try {
        auto request_data = json::parse(req.body);

        if (!request_data.contains("to_user") || !request_data.contains("message")) {
            send_error_response(res, 400, "to_user and message are required");
            return;
        }

        std::string target_user = request_data["to_user"];
        std::string message = request_data["message"];

        json message_obj = {
            {"from", username},
            {"to", target_user},
            {"message", message},
            {"timestamp", std::time(nullptr)}
        };

        send_message_to_user(target_user, message_obj);

        json response = {
            {"sent", true},
            {"to", target_user},
            {"message", message}
        };

        send_json_response(res, 200, response);
        LOG_INFO("Message sent from " + username + " to " + target_user);

    } catch (const json::exception& e) {
        send_error_response(res, 400, "Invalid JSON format");
        LOG_ERROR("JSON parsing error in send message: " + std::string(e.what()));
    }
}

void WebSocketService::handle_broadcast_message(const httplib::Request& req, httplib::Response& res) {
    std::string username;
    if (!validate_auth_token(req, username)) {
        send_error_response(res, 401, "Authentication required");
        return;
    }

    try {
        auto request_data = json::parse(req.body);

        if (!request_data.contains("message")) {
            send_error_response(res, 400, "message is required");
            return;
        }

        std::string message = request_data["message"];

        json message_obj = {
            {"from", username},
            {"message", message},
            {"type", "broadcast"},
            {"timestamp", std::time(nullptr)}
        };

        broadcast_message_to_all(message_obj);

        json response = {
            {"broadcast", true},
            {"message", message}
        };

        send_json_response(res, 200, response);
        LOG_INFO("Broadcast message sent by " + username);

    } catch (const json::exception& e) {
        send_error_response(res, 400, "Invalid JSON format");
        LOG_ERROR("JSON parsing error in broadcast: " + std::string(e.what()));
    }
}

void WebSocketService::handle_disconnect_user(const httplib::Request& req, httplib::Response& res) {
    std::string username;
    if (!validate_auth_token(req, username)) {
        send_error_response(res, 401, "Authentication required");
        return;
    }

    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = user_connections_.find(username);
    if (it != user_connections_.end()) {
        for (const auto& connection_id : it->second) {
            connections_.erase(connection_id);
        }
        user_connections_.erase(it);
    }

    notify_user_status_change(username, false);

    json response = {
        {"disconnected", true},
        {"user_id", username}
    };

    send_json_response(res, 200, response);
    LOG_INFO("User disconnected: " + username);
}

std::string WebSocketService::generate_connection_id() {
    std::stringstream ss;
    ss << "conn_" << connection_counter_++ << "_" << std::time(nullptr);
    return ss.str();
}

void WebSocketService::add_connection(const std::string& user_id, const std::string& connection_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    WebSocketConnection connection = {
        user_id,
        connection_id,
        std::time(nullptr),
        std::time(nullptr),
        true
    };

    connections_[connection_id] = connection;
    user_connections_[user_id].insert(connection_id);

    LOG_INFO("Connection added: " + connection_id + " for user: " + user_id);
}

void WebSocketService::remove_connection(const std::string& connection_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    auto it = connections_.find(connection_id);
    if (it != connections_.end()) {
        std::string user_id = it->second.user_id;
        connections_.erase(it);

        auto user_it = user_connections_.find(user_id);
        if (user_it != user_connections_.end()) {
            user_it->second.erase(connection_id);
            if (user_it->second.empty()) {
                user_connections_.erase(user_it);
            }
        }

        LOG_INFO("Connection removed: " + connection_id);
    }
}

void WebSocketService::cleanup_inactive_connections() {
    // В реальном приложении здесь была бы логика очистки неактивных соединений
    LOG_INFO("Cleanup inactive connections (placeholder)");
}

void WebSocketService::send_message_to_user(const std::string& target_user, const json& message) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    auto it = user_connections_.find(target_user);
    if (it != user_connections_.end() && !it->second.empty()) {
        LOG_INFO("Message would be sent to user: " + target_user + " (simulated)");
        // В реальном WebSocket здесь отправка сообщения
    } else {
        LOG_WARNING("User not found or offline: " + target_user);
    }
}

void WebSocketService::broadcast_message_to_all(const json& message) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    int sent_count = 0;
    for (const auto& [user_id, connection_ids] : user_connections_) {
        if (!connection_ids.empty()) {
            sent_count++;
            // В реальном WebSocket здесь отправка сообщения
        }
    }

    LOG_INFO("Broadcast message sent to " + std::to_string(sent_count) + " users (simulated)");
}

void WebSocketService::notify_user_status_change(const std::string& user_id, bool is_online) {
    json notification = {
        {"type", "user_status"},
        {"user_id", user_id},
        {"is_online", is_online},
        {"timestamp", std::time(nullptr)}
    };

    broadcast_message_to_all(notification);
    LOG_INFO("User status change broadcasted: " + user_id + " -> " + (is_online ? "online" : "offline"));
}

bool WebSocketService::validate_auth_token(const httplib::Request& req, std::string& username) {
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
            return true; // В реальном приложении проверили бы существование пользователя
        }
    }

    return false;
}