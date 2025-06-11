#pragma once

#include "common/http_service.h"
#include "handlers/message_handlers.h"
#include "data/message_manager.h"
#include <memory>

class MessageService : public HttpService {
public:
    explicit MessageService(int port = 8003);
    ~MessageService() override = default;

private:
    void setup_routes() override;

    std::shared_ptr<MessageManager> message_manager_;
    std::unique_ptr<MessageHandlers> handlers_;
};