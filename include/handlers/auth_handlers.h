#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>
#include "common/http_service.h"

using json = nlohmann::json;

class AuthHandlers {
public:
    static void handle_login(const httplib::Request& req, httplib::Response& res);
    static void handle_register(const httplib::Request& req, httplib::Response& res);
    static void handle_verify_token(const httplib::Request& req, httplib::Response& res);
    static void handle_refresh_token(const httplib::Request& req, httplib::Response& res);
    static void handle_logout(const httplib::Request& req, httplib::Response& res);

private:
    static bool validate_credentials(const std::string& username, const std::string& password);
    static json create_user_response(const std::string& username, const std::string& token);
    static void send_json_response(httplib::Response& res, int status, const json& data);
    static void send_error_response(httplib::Response& res, int status, const std::string& message);
};