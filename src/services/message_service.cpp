#include "services/message_service.h"
#include "common/logger.h"
#include <algorithm>
#include <sstream>

MessageService::MessageService(int port)
    : HttpService("MessageService", port), message_counter_(1) {
    create_sample_messages();
}

void MessageService::setup_routes() {
    auto& server = get_server();

    // Message endpoints
    server.Post("/api/messages/send", [this](const httplib::Request& req, httplib::Response& res) {
        handle_send_message(req, res);
    });

    server.Get("/api/conversations", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_conversations(req, res);
    });

    server.Get("/api/conversations/(.*)/messages", [this](const httplib::Request& req, httplib::Response& res) {
        handle_get_messages(req, res);
    });

    server.Put("/api/messages/(.*)/read", [this](const httplib::Request& req, httplib::Response& res) {
        handle_mark_as_read(req, res);
    });

    server.Delete("/api/messages/(.*)", [this](const httplib::Request& req, httplib::Response& res) {
        handle_delete_message(req, res);
    });

    LOG_INFO("Message Service routes configured");
}

void MessageService::handle_send_message(const httplib::Request& req, httplib::Response& res) {
    std::string from_user;
    if (!validate_auth_token(req, from_user)) {
        send_error_response(res, 401, "Authentication required");
        return;
    }

    try {
        auto request_data = json::parse(req.body);

        if (!request_data.contains("to_user") || !request_data.contains("content")) {
            send_error_response(res, 400, "to_user and content are required");
            return;
        }

        std::string to_user = request_data["to_user"];
        std::string content = request_data["content"];

        if (content.empty() || content.length() > 1000) {
            send_error_response(res, 400, "Message content must be 1-1000 characters");
            return;
        }

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

        std::lock_guard<std::mutex> lock(conversations_mutex_);
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

        json response = message_to_json(message);
        response["conversation_id"] = conv_id;

        send_json_response(res, 201, response);
        LOG_INFO("Message sent from " + from_user + " to " + to_user);

    } catch (const json::exception& e) {
        send_error_response(res, 400, "Invalid JSON format");
        LOG_ERROR("JSON parsing error in send message: " + std::string(e.what()));
    }
}

void MessageService::handle_get_conversations(const httplib::Request& req, httplib::Response& res) {
    std::string username;
    if (!validate_auth_token(req, username)) {
        send_error_response(res, 401, "Authentication required");
        return;
    }

    std::lock_guard<std::mutex> lock(conversations_mutex_);
    json conversations_array = json::array();

    for (const auto& [conv_id, conversation] : conversations_) {
        // Check if user is participant
        auto& participants = conversation.participants;
        if (std::find(participants.begin(), participants.end(), username) != participants.end()) {
            json conv_info = {
                {"id", conversation.id},
                {"participants", conversation.participants},
                {"last_activity", conversation.last_activity},
                {"message_count", conversation.messages.size()}
            };

            // Add last message preview
            if (!conversation.messages.empty()) {
                const auto& last_msg = conversation.messages.back();
                conv_info["last_message"] = {
                    {"content", last_msg.content.substr(0, 50) + (last_msg.content.length() > 50 ? "..." : "")},
                    {"from", last_msg.from_user},
                    {"timestamp", last_msg.timestamp}
                };
            }

            conversations_array.push_back(conv_info);
        }
    }

    // Sort by last activity
    std::sort(conversations_array.begin(), conversations_array.end(),
        [](const json& a, const json& b) {
            return a["last_activity"] > b["last_activity"];
        });

    json response = {
        {"conversations", conversations_array},
        {"total", conversations_array.size()}
    };

    send_json_response(res, 200, response);
    LOG_INFO("Conversations retrieved for user: " + username);
}

void MessageService::handle_get_messages(const httplib::Request& req, httplib::Response& res) {
    std::string username;
    if (!validate_auth_token(req, username)) {
        send_error_response(res, 401, "Authentication required");
        return;
    }

    std::string conv_id = req.matches[1];

    std::lock_guard<std::mutex> lock(conversations_mutex_);
    auto it = conversations_.find(conv_id);
    if (it == conversations_.end()) {
        send_error_response(res, 404, "Conversation not found");
        return;
    }

    // Check if user is participant
    auto& participants = it->second.participants;
    if (std::find(participants.begin(), participants.end(), username) == participants.end()) {
        send_error_response(res, 403, "Access denied to this conversation");
        return;
    }

    json messages_array = json::array();
    for (const auto& message : it->second.messages) {
        messages_array.push_back(message_to_json(message));
    }

    json response = {
        {"conversation_id", conv_id},
        {"messages", messages_array},
        {"total", messages_array.size()}
    };

    send_json_response(res, 200, response);
    LOG_INFO("Messages retrieved for conversation: " + conv_id + " by user: " + username);
}

void MessageService::handle_mark_as_read(const httplib::Request& req, httplib::Response& res) {
    std::string username;
    if (!validate_auth_token(req, username)) {
        send_error_response(res, 401, "Authentication required");
        return;
    }

    std::string message_id = req.matches[1];

    std::lock_guard<std::mutex> lock(conversations_mutex_);
    bool found = false;

    for (auto& [conv_id, conversation] : conversations_) {
        for (auto& message : conversation.messages) {
            if (message.id == message_id && message.to_user == username) {
                message.is_read = true;
                found = true;
                break;
            }
        }
        if (found) break;
    }

    if (!found) {
        send_error_response(res, 404, "Message not found or access denied");
        return;
    }

    json response = {
        {"message_id", message_id},
        {"marked_as_read", true}
    };

    send_json_response(res, 200, response);
    LOG_INFO("Message marked as read: " + message_id + " by user: " + username);
}

void MessageService::handle_delete_message(const httplib::Request& req, httplib::Response& res) {
    std::string username;
    if (!validate_auth_token(req, username)) {
        send_error_response(res, 401, "Authentication required");
        return;
    }

    std::string message_id = req.matches[1];

    std::lock_guard<std::mutex> lock(conversations_mutex_);
    bool found = false;

    for (auto& [conv_id, conversation] : conversations_) {
        auto& messages = conversation.messages;
        auto it = std::find_if(messages.begin(), messages.end(),
            [&](const Message& msg) {
                return msg.id == message_id && msg.from_user == username;
            });

        if (it != messages.end()) {
            messages.erase(it);
            found = true;
            break;
        }
    }

    if (!found) {
        send_error_response(res, 404, "Message not found or access denied");
        return;
    }

    json response = {
        {"message_id", message_id},
        {"deleted", true}
    };

    send_json_response(res, 200, response);
    LOG_INFO("Message deleted: " + message_id + " by user: " + username);
}

std::string MessageService::generate_message_id() {
    return "msg_" + std::to_string(std::time(nullptr)) + "_" + std::to_string(message_counter_++);
}

std::string MessageService::get_conversation_id(const std::string& user1, const std::string& user2) {
    std::vector<std::string> users = {user1, user2};
    std::sort(users.begin(), users.end());
    return "conv_" + users[0] + "_" + users[1];
}

json MessageService::message_to_json(const Message& message) {
    return {
        {"id", message.id},
        {"from_user", message.from_user},
        {"to_user", message.to_user},
        {"content", message.content},
        {"timestamp", message.timestamp},
        {"is_read", message.is_read}
    };
}

json MessageService::conversation_to_json(const Conversation& conversation) {
    json messages_array = json::array();
    for (const auto& message : conversation.messages) {
        messages_array.push_back(message_to_json(message));
    }

    return {
        {"id", conversation.id},
        {"participants", conversation.participants},
        {"messages", messages_array},
        {"last_activity", conversation.last_activity}
    };
}

bool MessageService::validate_auth_token(const httplib::Request& req, std::string& username) {
    std::string auth_header = req.get_header_value("Authorization");

    if (auth_header.empty() || auth_header.substr(0, 7) != "Bearer ") {
        return false;
    }

    std::string token = auth_header.substr(7);

    // Simple token validation
    if (token.substr(0, 4) == "jwt_") {
        size_t start = 4;
        size_t end = token.find('_', start);
        if (end != std::string::npos) {
            username = token.substr(start, end - start);
            return !username.empty();
        }
    }

    return false;
}

void MessageService::create_sample_messages() {
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

    LOG_INFO("Sample messages created");
}