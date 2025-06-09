#pragma once
#include "common/http_service.h"

class AuthService final : public HttpService {
public:
    explicit AuthService(int port = 8001);

protected:
    void setup_routes() override;

private:
    // Route handlers
    static void handle_login(const httplib::Request& req, httplib::Response& res);
    static void handle_register(const httplib::Request& req, httplib::Response& res);

    static void handle_verify_token(const httplib::Request& req, httplib::Response& res);

    static void handle_refresh_token(const httplib::Request& req, httplib::Response& res);

    static void handle_logout(const httplib::Request& req, httplib::Response& res);

    // Helper methods
    static bool validate_credentials(const std::string& username, const std::string& password);

    static std::string generate_jwt_token(const std::string& username);

    static bool verify_jwt_token(const std::string& token);

    static json create_user_response(const std::string& username, const std::string& token);
};