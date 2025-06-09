#include <iostream>
#include <string>

#include "common/logger.h"
#include "services/auth_service.h"

int main() {
    LOG_INFO("=== Messenger Gateway ===");
    LOG_INFO("🚀 Starting messenger backend...");

    try {
        // Create authentication service
        auto auth_service = std::make_unique<AuthService>(8001);

        // Start service
        auth_service->start();

        // Wait some time to observe the work
        LOG_INFO("⏰ Service will run for 15 seconds...");
        std::this_thread::sleep_for(std::chrono::seconds(15));

        // Stop service
        auth_service->stop();

        LOG_INFO("✅ Demo completed successfully!");

    } catch (const std::exception& e) {
        LOG_ERROR("❌ Critical error: " + std::string(e.what()));
        return 1;
    }

    return 0;
}
