#pragma once
#include "common/service_base.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <memory>

using json = nlohmann::json;

class HttpService : public ServiceBase {
public:
    explicit HttpService(const std::string& service_name, int port);
    virtual ~HttpService() = default;

protected:
    void on_start() override;
    void on_stop() override;
    void run_service() override;

    virtual void setup_routes() = 0;

    static void send_json_response(httplib::Response& res, int status, const json& data);

    static void send_error_response(httplib::Response& res, int status, const std::string& message);

    httplib::Server& get_server() const { return *server_; }

private:
    std::unique_ptr<httplib::Server> server_;

    void setup_middleware() const;
    void log_request(const httplib::Request& req, const httplib::Response& res) const;
};