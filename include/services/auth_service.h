#pragma once

#include "common/http_service.h"

class AuthService : public HttpService {
public:
    explicit AuthService(int port);
    ~AuthService() override = default;

private:
    void setup_routes() override;
};