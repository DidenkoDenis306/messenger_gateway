#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <vector>

#include "common/logger.h"
#include "services/auth_service.h"
#include "services/user_service.h"
#include "services/message_service.h"

int main() {
    LOG_INFO("=== Messenger Gateway ===");
    LOG_INFO("Starting messenger backend services...");

    try {
        // Create all services
        auto auth_service = std::make_unique<AuthService>(8001);
        auto user_service = std::make_unique<UserService>(8002);
        auto message_service = std::make_unique<MessageService>(8003);

        // Start all services
        LOG_INFO("Starting Auth Service...");
        auth_service->start();

        LOG_INFO("Starting User Service...");
        user_service->start();

        LOG_INFO("Starting Message Service...");
        message_service->start();

        LOG_INFO("=== All Services Started ===");
        LOG_INFO("Auth Service:    http://localhost:8001");
        LOG_INFO("User Service:    http://localhost:8002");
        LOG_INFO("Message Service: http://localhost:8003");
        LOG_INFO("");
        LOG_INFO("Available endpoints:");
        LOG_INFO("Auth Service:");
        LOG_INFO("  POST /api/auth/login");
        LOG_INFO("  POST /api/auth/register");
        LOG_INFO("  POST /api/auth/verify");
        LOG_INFO("  GET  /health");
        LOG_INFO("");
        LOG_INFO("User Service:");
        LOG_INFO("  GET  /api/users/profile");
        LOG_INFO("  PUT  /api/users/profile");
        LOG_INFO("  GET  /api/users");
        LOG_INFO("  GET  /api/users/search?q=<query>");
        LOG_INFO("  POST /api/users/status");
        LOG_INFO("  GET  /health");
        LOG_INFO("");
        LOG_INFO("Message Service:");
        LOG_INFO("  POST /api/messages/send");
        LOG_INFO("  GET  /api/conversations");
        LOG_INFO("  GET  /api/conversations/<id>/messages");
        LOG_INFO("  PUT  /api/messages/<id>/read");
        LOG_INFO("  DELETE /api/messages/<id>");
        LOG_INFO("  GET  /health");
        LOG_INFO("");
        LOG_INFO("Press Ctrl+C to stop all services...");

        // Keep running until interrupted
        while (auth_service->is_running() &&
               user_service->is_running() &&
               message_service->is_running()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Stop all services
        LOG_INFO("Shutting down services...");
        auth_service->stop();
        user_service->stop();
        message_service->stop();

        LOG_INFO("Messenger Gateway shutdown complete");

    } catch (const std::exception& e) {
        LOG_ERROR("Critical error: " + std::string(e.what()));
        return 1;
    }

    return 0;
}