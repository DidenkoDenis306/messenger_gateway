#pragma once

#include "common/http_service.h"
#include "handlers/websocket_handlers.h"
#include "data/connection_manager.h"
#include <memory>
#include <thread>

class WebSocketService : public HttpService {
public:
    explicit WebSocketService(int port);
    ~WebSocketService() override;

private:
    void setup_routes() override;
    void on_start() override;
    void on_stop() override;

    void cleanup_worker();

    std::shared_ptr<ConnectionManager> connection_manager_;
    std::unique_ptr<WebSocketHandlers> handlers_;
    std::thread cleanup_thread_;
    bool should_cleanup_;
};