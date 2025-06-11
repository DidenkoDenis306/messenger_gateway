#include "services/auth_service.h"
#include "handlers/auth_handlers.h"
#include "common/logger.h"

AuthService::AuthService(const int port) : HttpService("AuthService", port) {
}

void AuthService::setup_routes() {
    auto& server = get_server();

    // Authentication endpoints
    server.Post("/api/auth/login", [](const httplib::Request& req, httplib::Response& res) {
        AuthHandlers::handle_login(req, res);
    });

    server.Post("/api/auth/register", [](const httplib::Request& req, httplib::Response& res) {
        AuthHandlers::handle_register(req, res);
    });

    server.Post("/api/auth/verify", [](const httplib::Request& req, httplib::Response& res) {
        AuthHandlers::handle_verify_token(req, res);
    });

    server.Post("/api/auth/refresh", [](const httplib::Request& req, httplib::Response& res) {
        AuthHandlers::handle_refresh_token(req, res);
    });

    server.Post("/api/auth/logout", [](const httplib::Request& req, httplib::Response& res) {
        AuthHandlers::handle_logout(req, res);
    });

    LOG_INFO("Auth Service routes configured");
}