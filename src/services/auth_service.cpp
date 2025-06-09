#include "services/auth_service.h"
#include "common/logger.h"
#include <chrono>
#include <thread>

AuthService::AuthService(int port) : ServiceBase("AuthService", port) {
}

void AuthService::on_start() {
    LOG_INFO("🔐 Initialization Auth Service...");
    LOG_INFO("📋 Processing users database...");
    LOG_INFO("🔑 Loading JWT secrets...");
}

void AuthService::on_stop() {
    LOG_INFO("🔐 Closing connections with Auth Service...");
}

void AuthService::run_service() {
    LOG_INFO("🔐 Auth Service has started request proccesing");

    // Service working simulation
    int request_count = 0;
    while (is_running()) {
        simulate_auth_requests();
        request_count++;

        // Imitation request processing every 2 seconds
        std::this_thread::sleep_for(std::chrono::seconds(2));

        if (request_count >= 5) {
            break;
        }
    }

    LOG_INFO("🔐 Auth Service finished working");
}

void AuthService::simulate_auth_requests() {
    static int request_id = 1;

    const std::string requests[] = {
        "Login request for user 'user" + std::to_string(request_id) + "'",
        "JWT token verification",
        "New user registration",
        "Access token refresh"
    };

    std::string request = requests[request_id % 4];
    LOG_INFO("🔐 [Auth] Processing: " + request);

    // Simulate processing time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    LOG_INFO("✅ [Auth] Request processed successfully");
    request_id++;
}