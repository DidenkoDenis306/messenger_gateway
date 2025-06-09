#include <iostream>
#include <memory>
#include <chrono>
#include <thread>

#include "common/logger.h"
#include "services/auth_service.h"

int main() {
    LOG_INFO("=== Messenger Gateway ===");
    LOG_INFO("Starting messenger backend...");

    try {
        int port = 8001;
        // Create Auth Service
        const auto auth_service = std::make_unique<AuthService>(port);

        // Start the service
        auth_service->start();

        LOG_INFO(std::format("Auth Service is running on http://localhost:{}", std::to_string(port)));
        LOG_INFO("Available endpoints:");
        LOG_INFO("  POST /api/auth/login");
        LOG_INFO("  POST /api/auth/register");
        LOG_INFO("  POST /api/auth/verify");
        LOG_INFO("  POST /api/auth/refresh");
        LOG_INFO("  POST /api/auth/logout");
        LOG_INFO("  GET  /health");

        LOG_INFO("Press Ctrl+C to stop the server...");

        // Keep running until interrupted
        while (auth_service->is_running()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Stop the service
        auth_service->stop();

        LOG_INFO("Messenger Gateway shutdown complete");

    } catch (const std::exception& e) {
        LOG_ERROR("Critical error: " + std::string(e.what()));
        return 1;
    }

    return 0;
}