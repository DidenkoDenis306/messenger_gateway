#include "common/http_service.h"
#include "common/logger.h"
#include <sstream>

HttpService::HttpService(const std::string& service_name, int port)
    : ServiceBase(service_name, port), server_(std::make_unique<httplib::Server>()){
}

void HttpService::on_start() {
    LOG_INFO("Setting up HTTP server for " + get_name());
    setup_middleware();
    setup_routes();
    LOG_INFO("HTTP routes configured for " + get_name());
}

void HttpService::on_stop() {
    LOG_INFO("Stopping HTTP server for " + get_name());
    if (server_) {
        server_->stop();
    }
}

void HttpService::run_service() {
    LOG_INFO("Starting HTTP server on port " + std::to_string(get_port()));

    if (!server_->listen("0.0.0.0", get_port())) {
        LOG_ERROR("Failed to start HTTP server on port " + std::to_string(get_port()));
        return;
    }

    LOG_INFO("HTTP server stopped for " + get_name());
}

void HttpService::setup_middleware() const {
    // CORS middleware
    server_->set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
       res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
       res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
       return httplib::Server::HandlerResponse::Unhandled;
    });

    // Logging middleware
    server_->set_logger([this](const httplib::Request& req, const httplib::Response& res) {
        log_request(req, res);
    });

    // Health check endpoint (common for all services)
    server_->Get("/health", [this](const httplib::Request& req, httplib::Response& res) {
        const json health_data = {
            {"service", get_name()},
            {"status", "healthy"},
            {"port", get_port()},
            {"timestamp", std::time(nullptr)}
        };
        send_json_response(res, 200, health_data);
    });
}


void HttpService::send_json_response(httplib::Response& res, int status, const json& data) {
    res.status = status;
    res.set_content(data.dump(2), "application/json");
}

void HttpService::send_error_response(httplib::Response& res, int status, const std::string& message) {
    const json error_data = {
        {"error", true},
        {"message", message},
        {"status", status}
    };
    send_json_response(res, status, error_data);
}

void HttpService::log_request(const httplib::Request& req, const httplib::Response& res) const {
    std::ostringstream log_msg;
    log_msg << "[" << get_name() << "] "
            << req.method << " " << req.path
            << " -> " << res.status;

    if (!req.body.empty()) {
        log_msg << " (body: " << req.body.length() << " bytes)";
    }

    LOG_INFO(log_msg.str());
}

