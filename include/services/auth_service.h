#pragma once
#include "common/service_base.h"

class AuthService final : public ServiceBase {
public:
    explicit AuthService(int port = 8001);

protected:
    void on_start() override;
    void on_stop() override;
    void run_service() override;

private:
    static void simulate_auth_requests();
};