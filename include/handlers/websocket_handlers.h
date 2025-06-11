#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>
#include "data/connection_manager.h"
#include <memory>

using json = nlohmann::json;

class WebSocketHandlers {
public:
    explicit WebSocketHandlers(std::shared_ptr<ConnectionManager> connection_manager);

    void handle_get_stats(const httplib::Request& req, httplib::Response& res);
    void handle_get_online_users(const httplib::Request& req, httplib::Response& res);
    void handle_connect_user(const httplib::Request& req, httplib::Response& res);
    void handle_send_message(const httplib::Request& req, httplib::Response& res);
    void handle_broadcast_message(const httplib::Request& req, httplib::Response& res);
    void handle_disconnect_user(const httplib::Request& req, httplib::Response& res);

private:
    std::shared_ptr<ConnectionManager> connection_manager_;

    void send_message_to_user(const std::string& target_user, const json& message);
    void broadcast_message_to_all(const json& message);
    void notify_user_status_change(const std::string& user_id, bool is_online);

    void send_json_response(httplib::Response& res, int status, const json& data);
    void send_error_response(httplib::Response& res, int status, const std::string& message);
};