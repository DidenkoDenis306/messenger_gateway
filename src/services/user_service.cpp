#include "services/user_service.h"
#include "common/logger.h"

UserService::UserService(int port) : HttpService("UserService", port) {
    user_manager_ = std::make_shared<UserManager>();
    handlers_ = std::make_unique<UserHandlers>(user_manager_);
}

void UserService::setup_routes() {
    auto& server = get_server();

    // User management endpoints
    server.Get("/api/users/profile", [this](const httplib::Request& req, httplib::Response& res) {
        handlers_->handle_get_user(req, res);
    });

    server.Put("/api/users/profile", [this](const httplib::Request& req, httplib::Response& res) {
        handlers_->handle_update_user(req, res);
    });

    server.Get("/api/users", [this](const httplib::Request& req, httplib::Response& res) {
        handlers_->handle_get_users(req, res);
    });

    server.Get("/api/users/search", [this](const httplib::Request& req, httplib::Response& res) {
        handlers_->handle_search_users(req, res);
    });

    server.Post("/api/users/status", [this](const httplib::Request& req, httplib::Response& res) {
        handlers_->handle_set_online_status(req, res);
    });

    LOG_INFO("User Service routes configured");
}