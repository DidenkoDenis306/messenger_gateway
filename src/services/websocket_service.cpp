#include "services/websocket_service.h"
#include "common/logger.h"
#include <chrono>

WebSocketService::WebSocketService(int port)
    : HttpService("WebSocketService", port), should_cleanup_(false) {
    connection_manager_ = std::make_shared<ConnectionManager>();
    handlers_ = std::make_unique<WebSocketHandlers>(connection_manager_);
}

WebSocketService::~WebSocketService() {
    should_cleanup_ = false;
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
}

void WebSocketService::setup_routes() {
    auto& server = get_server();

    // WebSocket management endpoints
    server.Get("/api/websocket/stats", [this](const httplib::Request& req, httplib::Response& res) {
        handlers_->handle_get_stats(req, res);
    });

    server.Get("/api/websocket/online", [this](const httplib::Request& req, httplib::Response& res) {
        handlers_->handle_get_online_users(req, res);
    });

    server.Post("/api/websocket/connect", [this](const httplib::Request& req, httplib::Response& res) {
        handlers_->handle_connect_user(req, res);
    });

    server.Post("/api/websocket/send", [this](const httplib::Request& req, httplib::Response& res) {
        handlers_->handle_send_message(req, res);
    });

    server.Post("/api/websocket/broadcast", [this](const httplib::Request& req, httplib::Response& res) {
        handlers_->handle_broadcast_message(req, res);
    });

    server.Post("/api/websocket/disconnect", [this](const httplib::Request& req, httplib::Response& res) {
        handlers_->handle_disconnect_user(req, res);
    });

    LOG_INFO("WebSocket Service routes configured");
}

void WebSocketService::on_start() {
    HttpService::on_start();

    // Start cleanup thread
    should_cleanup_ = true;
    cleanup_thread_ = std::thread(&WebSocketService::cleanup_worker, this);
    LOG_INFO("WebSocket cleanup thread started");
}

void WebSocketService::on_stop() {
    should_cleanup_ = false;
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
    LOG_INFO("WebSocket cleanup thread stopped");

    HttpService::on_stop();
}

void WebSocketService::cleanup_worker() {
    while (should_cleanup_) {
        connection_manager_->cleanup_inactive_connections();
        std::this_thread::sleep_for(std::chrono::seconds(30)); // Cleanup every 30 seconds
    }
}