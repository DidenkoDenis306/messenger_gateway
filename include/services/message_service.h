#pragma once
#include "common/http_service.h"
#include <vector>
#include <unordered_map>

struct Message {
    std::string id;
    std::string from_user;
    std::string to_user;
    std::string content;
    std::time_t timestamp;
    bool is_read;
};

struct Conversation {
    std::string id;
    std::vector<std::string> participants;
    std::vector<Message> messages;
    std::time_t last_activity;
};

class MessageService final : public HttpService {
public:
    explicit MessageService(int port = 8003);

protected:
    void setup_routes() override;

private:
    // Route handlers
    void handle_send_message(const httplib::Request& req, httplib::Response& res);
    void handle_get_conversations(const httplib::Request& req, httplib::Response& res);
    void handle_get_messages(const httplib::Request& req, httplib::Response& res);
    void handle_mark_as_read(const httplib::Request& req, httplib::Response& res);
    void handle_delete_message(const httplib::Request& req, httplib::Response& res);

    // Helper methods
    std::string generate_message_id();
    std::string get_conversation_id(const std::string& user1, const std::string& user2);
    json message_to_json(const Message& message);
    json conversation_to_json(const Conversation& conversation);
    bool validate_auth_token(const httplib::Request& req, std::string& username);
    void create_sample_messages();

    // In-memory storage
    std::unordered_map<std::string, Conversation> conversations_;
    mutable std::mutex conversations_mutex_;
    int message_counter_;
};