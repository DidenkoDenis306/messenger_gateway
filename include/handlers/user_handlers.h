#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>
#include "data/user_manager.h"
#include <memory>

using json = nlohmann::json;

class UserHandlers {
public:
    explicit UserHandlers(std::shared_ptr<UserManager> user_manager);

    void handle_get_user(const httplib::Request& req, httplib::Response& res);
    void handle_update_user(const httplib::Request& req, httplib::Response& res);
    void handle_get_users(const httplib::Request& req, httplib::Response& res);
    void handle_search_users(const httplib::Request& req, httplib::Response& res);
    void handle_set_online_status(const httplib::Request& req, httplib::Response& res);

private:
    std::shared_ptr<UserManager> user_manager_;

    void send_json_response(httplib::Response& res, int status, const json& data);
    void send_error_response(httplib::Response& res, int status, const std::string& message);
};