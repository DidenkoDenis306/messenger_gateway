#pragma once

#include <nlohmann/json.hpp>
#include <vector>
#include <unordered_map>
#include <string>
#include <mutex>
#include <optional>

using json = nlohmann::json;

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

class MessageManager {
public:
    MessageManager();

    // Message operations
    std::string send_message(const std::string& from_user, const std::string& to_user, const std::string& content);
    bool mark_message_as_read(const std::string& message_id, const std::string& username);
    bool delete_message(const std::string& message_id, const std::string& username);

    // Conversation operations
    std::vector<Conversation> get_user_conversations(const std::string& username);
    std::optional<Conversation> get_conversation(const std::string& conversation_id, const std::string& username);
    std::vector<Message> get_conversation_messages(const std::string& conversation_id, const std::string& username);

    // Utility
    json message_to_json(const Message& message);
    json conversation_to_json(const Conversation& conversation, bool include_messages = false);

private:
    std::string generate_message_id();
    std::string get_conversation_id(const std::string& user1, const std::string& user2);
    bool is_user_participant(const Conversation& conversation, const std::string& username);
    void create_sample_messages();

    std::unordered_map<std::string, Conversation> conversations_;
    std::mutex conversations_mutex_;
    int message_counter_;
};