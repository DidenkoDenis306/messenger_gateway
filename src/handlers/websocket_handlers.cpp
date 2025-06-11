#include "handlers/websocket_handlers.h"
#include "common/auth_middleware.h"
#include "common/request_validator.h"
#include "common/logger.h"

WebSocketHandlers::WebSocketHandlers(std::shared_ptr<ConnectionManager> connection_manager)
    : connection_manager_(connection_manager) {
}

void WebSocketHandlers::handle_get_stats(const httplib::Request& req, httplib::Response& res) {
    json stats = connection_manager_->get_stats();
    send_json_response(res, 200, stats);
    LOG_INFO("WebSocket stats requested");
}

void WebSocketHandlers::handle_get_online_users(const httplib::Request& req, httplib::Response& res) {
    auto online_users = connection_manager_->get_online_users();

    json response = {
        {"online_users", online_users},
        {"count", online_users.size()},
        {"timestamp", std::time(nullptr)}
    };

    send_json_response(res, 200, response);
    LOG_INFO("Online users list requested (" + std::to_string(online_users.size()) + " users)");
}

void WebSocketHandlers::handle_connect_user(const httplib::Request& req, httplib::Response& res) {
    std::string user_id;

    // Try to get user_id from URL parameters
    user_id = req.get_param_value("user_id");

    // If not found and there's JSON body, try from there
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

    // Try from authorization header
    if (user_id.empty()) {
        auto auth_result = AuthMiddleware::validate_token(req);
        if (auth_result.is_valid) {
            user_id = auth_result.username;
        }
    }

    if (user_id.empty()) {
        send_error_response(res, 400, "user_id parameter is required (in URL params, JSON body, or extracted from auth token)");
        return;
    }

    LOG_INFO("User connecting: " + user_id);

    std::string connection_id = connection_manager_->add_connection(user_id);

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

void WebSocketHandlers::handle_send_message(const httplib::Request& req, httplib::Response& res) {
    auto auth_result = AuthMiddleware::validate_token(req);
    if (!auth_result.is_valid) {
        send_error_response(res, 401, auth_result.error_message);
        return;
    }

    auto validation = RequestValidator::validate_json_body(req, {"to_user", "message"});
    if (!validation.is_valid) {
        send_error_response(res, 400, validation.error_message);
        return;
    }

    const std::string target_user = validation.data["to_user"];
    const std::string message_text = validation.data["message"];

    json message = {
        {"from", auth_result.username},
        {"to", target_user},
        {"message", message_text},
        {"timestamp", std::time(nullptr)},
        {"type", "direct_message"}
    };

    send_message_to_user(target_user, message);

    json response = {
        {"sent", true},
        {"to", target_user},
        {"message", message_text}
    };

    send_json_response(res, 200, response);
    LOG_INFO("Message sent from " + auth_result.username + " to " + target_user);
}

void WebSocketHandlers::handle_broadcast_message(const httplib::Request& req, httplib::Response& res) {
    auto auth_result = AuthMiddleware::validate_token(req);
    if (!auth_result.is_valid) {
        send_error_response(res, 401, auth_result.error_message);
        return;
    }

    auto validation = RequestValidator::validate_json_body(req, {"message"});
    if (!validation.is_valid) {
        send_error_response(res, 400, validation.error_message);
        return;
    }

    const std::string message_text = validation.data["message"];

    json message = {
        {"from", auth_result.username},
        {"message", message_text},
        {"timestamp", std::time(nullptr)},
        {"type", "broadcast"}
    };

    broadcast_message_to_all(message);

    json response = {
        {"broadcast", true},
        {"message", message_text},
        {"sent_to", connection_manager_->get_active_users_count()}
    };

    send_json_response(res, 200, response);
    LOG_INFO("Broadcast message sent by " + auth_result.username);
}

void WebSocketHandlers::handle_disconnect_user(const httplib::Request& req, httplib::Response& res) {
    std::string connection_id = req.get_param_value("connection_id");
    std::string user_id = req.get_param_value("user_id");

    if (!connection_id.empty()) {
        if (connection_manager_->remove_connection(connection_id)) {
            json response = {
                {"disconnected", true},
                {"connection_id", connection_id}
            };
            send_json_response(res, 200, response);
            LOG_INFO("Connection disconnected: " + connection_id);
        } else {
            send_error_response(res, 404, "Connection not found");
        }
    } else if (!user_id.empty()) {
        if (connection_manager_->remove_user_connections(user_id)) {
            notify_user_status_change(user_id, false);
            json response = {
                {"disconnected", true},
                {"user_id", user_id}
            };
            send_json_response(res, 200, response);
            LOG_INFO("User disconnected: " + user_id);
        } else {
            send_error_response(res, 404, "User not found");
        }
    } else {
        send_error_response(res, 400, "Either connection_id or user_id parameter is required");
    }
}

void WebSocketHandlers::send_message_to_user(const std::string& target_user, const json& message) {
    // Simulate sending message to user (in real implementation, send via WebSocket)
    LOG_INFO("Message sent to user " + target_user + ": " + message.dump());
}

void WebSocketHandlers::broadcast_message_to_all(const json& message) {
    auto online_users = connection_manager_->get_online_users();
    LOG_INFO("Broadcast message sent to " + std::to_string(online_users.size()) + " users (simulated)");
}

void WebSocketHandlers::notify_user_status_change(const std::string& user_id, bool is_online) {
    json notification = {
        {"type", "user_status_change"},
        {"user_id", user_id},
        {"is_online", is_online},
        {"timestamp", std::time(nullptr)}
    };

    broadcast_message_to_all(notification);
    LOG_INFO("User status change broadcasted: " + user_id + " -> " + (is_online ? "online" : "offline"));
}

void WebSocketHandlers::send_json_response(httplib::Response& res, int status, const json& data) {
    res.status = status;
    res.set_content(data.dump(2), "application/json");
}

void WebSocketHandlers::send_error_response(httplib::Response& res, int status, const std::string& message) {
    const json error_data = {
        {"error", true},
        {"message", message},
        {"status", status}
    };
    send_json_response(res, status, error_data);
}