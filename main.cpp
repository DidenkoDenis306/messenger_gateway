#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <vector>

#include "common/logger.h"
#include "services/auth_service.h"
#include "services/user_service.h"
#include "services/message_service.h"
#include "services/websocket_service.h"

int main() {
    LOG_INFO("=== Messenger Gateway ===");
    LOG_INFO("Starting messenger backend services...");

    try {
        // Create all services
        auto auth_service = std::make_unique<AuthService>(8001);
        auto user_service = std::make_unique<UserService>(8002);
        auto message_service = std::make_unique<MessageService>(8003);
        auto websocket_service = std::make_unique<WebSocketService>(8004);

        // Start all services
        LOG_INFO("Starting Auth Service...");
        auth_service->start();

        LOG_INFO("Starting User Service...");
        user_service->start();

        LOG_INFO("Starting Message Service...");
        message_service->start();

        LOG_INFO("Starting WebSocket Service...");
        websocket_service->start();

        LOG_INFO("=== All Services Started ===");
        LOG_INFO("Auth Service:      http://localhost:8001");
        LOG_INFO("User Service:      http://localhost:8002");
        LOG_INFO("Message Service:   http://localhost:8003");
        LOG_INFO("WebSocket Service: http://localhost:8004");
        LOG_INFO("");
        LOG_INFO("WebSocket endpoints:");
        LOG_INFO("  GET  /api/websocket/stats");
        LOG_INFO("  GET  /api/websocket/online");
        LOG_INFO("  POST /api/websocket/connect?user_id=<id>");
        LOG_INFO("  POST /api/websocket/send?target_user=<user>&message=<msg>");
        LOG_INFO("  POST /api/websocket/broadcast?message=<msg>");

        // Keep running
        while (auth_service->is_running() &&
               user_service->is_running() &&
               message_service->is_running() &&
               websocket_service->is_running()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Stop all services
        LOG_INFO("Shutting down services...");
        auth_service->stop();
        user_service->stop();
        message_service->stop();
        websocket_service->stop();

        LOG_INFO("Messenger Gateway shutdown complete");

    } catch (const std::exception& e) {
        LOG_ERROR("Critical error: " + std::string(e.what()));
        return 1;
    }

    return 0;
}