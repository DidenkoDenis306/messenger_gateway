#include "services/message_service.h"
#include "common/logger.h"

MessageService::MessageService(int port) : HttpService("MessageService", port) {
    message_manager_ = std::make_shared<MessageManager>();
    handlers_ = std::make_unique<MessageHandlers>(message_manager_);
}

void MessageService::setup_routes() {
    auto& server = get_server();

    // Message endpoints
    server.Post("/api/messages/send", [this](const httplib::Request& req, httplib::Response& res) {
        handlers_->handle_send_message(req, res);
    });

    server.Get("/api/conversations", [this](const httplib::Request& req, httplib::Response& res) {
        handlers_->handle_get_conversations(req, res);
    });

    server.Get("/api/conversations/(.*)/messages", [this](const httplib::Request& req, httplib::Response& res) {
        handlers_->handle_get_messages(req, res);
    });

    server.Put("/api/messages/(.*)/read", [this](const httplib::Request& req, httplib::Response& res) {
        handlers_->handle_mark_as_read(req, res);
    });

    server.Delete("/api/messages/(.*)", [this](const httplib::Request& req, httplib::Response& res) {
        handlers_->handle_delete_message(req, res);
    });

    LOG_INFO("Message Service routes configured");
}