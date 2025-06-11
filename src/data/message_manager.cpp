#include "data/message_manager.h"
#include "common/logger.h"
#include <algorithm>
#include <sstream>

MessageManager::MessageManager() : message_counter_(1) {
    create_sample_messages();
}

std::string MessageManager::send_message(const std::string& from_user, const std::string& to_user, const std::string& content) {
    std::lock_guard<std::mutex> lock(conversations_mutex_);

    // Create message
    Message message = {
        generate_message_id(),
        from_user,
        to_user,
        content,
        std::time(nullptr),
        false
    };

    // Get or create conversation
    std::string conv_id = get_conversation_id(from_user, to_user);

    if (conversations_.find(conv_id) == conversations_.end()) {
        conversations_[conv_id] = {
            conv_id,
            {from_user, to_user},
            {},
            message.timestamp
        };
    }

    conversations_[conv_id].messages.push_back(message);
    conversations_[conv_id].last_activity = message.timestamp;

    LOG_INFO("Message sent: " + message.id + " from " + from_user + " to " + to_user);
    return message.id;
}

bool MessageManager::mark_message_as_read(const std::string& message_id, const std::string& username) {
    std::lock_guard<std::mutex> lock(conversations_mutex_);

    for (auto& [conv_id, conversation] : conversations_) {
        for (auto& message : conversation.messages) {
            if (message.id == message_id && message.to_user == username) {
                message.is_read = true;
                LOG_INFO("Message marked as read: " + message_id + " by " + username);
                return true;
            }
        }
    }

    return false;
}

bool MessageManager::delete_message(const std::string& message_id, const std::string& username) {
    std::lock_guard<std::mutex> lock(conversations_mutex_);

    for (auto& [conv_id, conversation] : conversations_) {
        auto& messages = conversation.messages;
        auto it = std::find_if(messages.begin(), messages.end(),
            [&](const Message& msg) {
                return msg.id == message_id && msg.from_user == username;
            });

        if (it != messages.end()) {
            messages.erase(it);
            LOG_INFO("Message deleted: " + message_id + " by " + username);
            return true;
        }
    }

    return false;
}

std::vector<Conversation> MessageManager::get_user_conversations(const std::string& username) {
    std::lock_guard<std::mutex> lock(conversations_mutex_);

    std::vector<Conversation> user_conversations;

    for (const auto& [conv_id, conversation] : conversations_) {
        if (is_user_participant(conversation, username)) {
            user_conversations.push_back(conversation);
        }
    }

    // Sort by last activity (most recent first)
    std::sort(user_conversations.begin(), user_conversations.end(),
        [](const Conversation& a, const Conversation& b) {
            return a.last_activity > b.last_activity;
        });

    return user_conversations;
}

std::optional<Conversation> MessageManager::get_conversation(const std::string& conversation_id, const std::string& username) {
    std::lock_guard<std::mutex> lock(conversations_mutex_);

    auto it = conversations_.find(conversation_id);
    if (it == conversations_.end()) {
        return std::nullopt;
    }

    if (!is_user_participant(it->second, username)) {
        return std::nullopt;
    }

    return it->second;
}

std::vector<Message> MessageManager::get_conversation_messages(const std::string& conversation_id, const std::string& username) {
    std::lock_guard<std::mutex> lock(conversations_mutex_);

    auto it = conversations_.find(conversation_id);
    if (it == conversations_.end() || !is_user_participant(it->second, username)) {
        return {};
    }

    return it->second.messages;
}

json MessageManager::message_to_json(const Message& message) {
    return {
        {"id", message.id},
        {"from_user", message.from_user},
        {"to_user", message.to_user},
        {"content", message.content},
        {"timestamp", message.timestamp},
        {"is_read", message.is_read}
    };
}

json MessageManager::conversation_to_json(const Conversation& conversation, bool include_messages) {
    json conv_json = {
        {"id", conversation.id},
        {"participants", conversation.participants},
        {"last_activity", conversation.last_activity},
        {"message_count", conversation.messages.size()}
    };

    if (include_messages) {
        json messages_array = json::array();
        for (const auto& message : conversation.messages) {
            messages_array.push_back(message_to_json(message));
        }
        conv_json["messages"] = messages_array;
    } else if (!conversation.messages.empty()) {
        // Add last message preview
        const auto& last_msg = conversation.messages.back();
        conv_json["last_message"] = {
            {"content", last_msg.content.substr(0, 50) + (last_msg.content.length() > 50 ? "..." : "")},
            {"from", last_msg.from_user},
            {"timestamp", last_msg.timestamp}
        };
    }

    return conv_json;
}

std::string MessageManager::generate_message_id() {
    return "msg_" + std::to_string(std::time(nullptr)) + "_" + std::to_string(message_counter_++);
}

std::string MessageManager::get_conversation_id(const std::string& user1, const std::string& user2) {
    std::vector<std::string> users = {user1, user2};
    std::sort(users.begin(), users.end());
    return "conv_" + users[0] + "_" + users[1];
}

bool MessageManager::is_user_participant(const Conversation& conversation, const std::string& username) {
    const auto& participants = conversation.participants;
    return std::find(participants.begin(), participants.end(), username) != participants.end();
}

void MessageManager::create_sample_messages() {
    std::time_t now = std::time(nullptr);

    // Create sample conversation
    std::string conv_id = get_conversation_id("alice", "bob");
    conversations_[conv_id] = {
        conv_id,
        {"alice", "bob"},
        {
            {"msg_1", "alice", "bob", "Hello Bob! How are you?", now - 3600, true},
            {"msg_2", "bob", "alice", "Hi Alice! I'm doing great, thanks!", now - 3500, true},
            {"msg_3", "alice", "bob", "That's wonderful to hear!", now - 3400, false}
        },
        now - 3400
    };

    LOG_INFO("Sample messages created for conversation: " + conv_id);
}