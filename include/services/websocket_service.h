#pragma once

#include "common/http_service.h"
#include "common/logger.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>
#include <mutex>
#include <thread>

using json = nlohmann::json;

struct WebSocketConnection {
    std::string user_id;
    std::string connection_id;
    std::time_t connected_at;
    std::time_t last_activity;
    bool is_active;
};

class WebSocketService : public HttpService {
public:
    explicit WebSocketService(int port);
    ~WebSocketService() override = default;

private:
    void setup_routes() override;

    // HTTP endpoints for WebSocket management
    void handle_get_stats(const httplib::Request& req, httplib::Response& res);
    void handle_get_online_users(const httplib::Request& req, httplib::Response& res);
    void handle_connect_user(const httplib::Request& req, httplib::Response& res);
    void handle_send_message(const httplib::Request& req, httplib::Response& res);
    void handle_broadcast_message(const httplib::Request& req, httplib::Response& res);
    void handle_disconnect_user(const httplib::Request& req, httplib::Response& res);

    // Connection management
    std::string generate_connection_id();
    void add_connection(const std::string& user_id, const std::string& connection_id);
    void remove_connection(const std::string& connection_id);
    void cleanup_inactive_connections();

    // Message handling
    void send_message_to_user(const std::string& target_user, const json& message);
    void broadcast_message_to_all(const json& message);
    void notify_user_status_change(const std::string& user_id, bool is_online);

    // Utility methods
    bool validate_auth_token(const httplib::Request& req, std::string& username);

    // Data
    std::unordered_map<std::string, WebSocketConnection> connections_; // connection_id -> connection
    std::unordered_map<std::string, std::unordered_set<std::string>> user_connections_; // user_id -> connection_ids

    std::mutex connections_mutex_;
    std::thread cleanup_thread_;
    bool should_cleanup_;

    size_t connection_counter_;
};