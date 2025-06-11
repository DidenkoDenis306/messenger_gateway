#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>
#include "data/message_manager.h"
#include <memory>

using json = nlohmann::json;

class MessageHandlers {
public:
    explicit MessageHandlers(std::shared_ptr<MessageManager> message_manager);

    void handle_send_message(const httplib::Request& req, httplib::Response& res);
    void handle_get_conversations(const httplib::Request& req, httplib::Response& res);
    void handle_get_messages(const httplib::Request& req, httplib::Response& res);
    void handle_mark_as_read(const httplib::Request& req, httplib::Response& res);
    void handle_delete_message(const httplib::Request& req, httplib::Response& res);

private:
    std::shared_ptr<MessageManager> message_manager_;

    bool validate_message_content(const std::string& content, std::string& error_message);
    void send_json_response(httplib::Response& res, int status, const json& data);
    void send_error_response(httplib::Response& res, int status, const std::string& message);
};