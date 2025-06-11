#pragma once

#include "common/http_service.h"
#include "handlers/user_handlers.h"
#include "data/user_manager.h"
#include <memory>

class UserService : public HttpService {
public:
    explicit UserService(int port);
    ~UserService() override = default;

private:
    void setup_routes() override;

    std::shared_ptr<UserManager> user_manager_;
    std::unique_ptr<UserHandlers> handlers_;
};