#include "handlers/message_handlers.h"
#include "common/auth_middleware.h"
#include "common/request_validator.h"
#include "common/logger.h"

MessageHandlers::MessageHandlers(std::shared_ptr<MessageManager> message_manager)
    : message_manager_(message_manager) {
}

void MessageHandlers::handle_send_message(const httplib::Request& req, httplib::Response& res) {
    auto auth_result = AuthMiddleware::validate_token(req);
    if (!auth_result.is_valid) {
        send_error_response(res, 401, auth_result.error_message);
        return;
    }

    auto validation = RequestValidator::validate_json_body(req, {"to_user", "content"});
    if (!validation.is_valid) {
        send_error_response(res, 400, validation.error_message);
        return;
    }

    const std::string to_user = validation.data["to_user"];
    const std::string content = validation.data["content"];

    // Validate message content
    std::string content_error;
    if (!validate_message_content(content, content_error)) {
        send_error_response(res, 400, content_error);
        return;
    }

    // Send message
    std::string message_id = message_manager_->send_message(auth_result.username, to_user, content);

    json response = {
        {"message_id", message_id},
        {"from_user", auth_result.username},
        {"to_user", to_user},
        {"content", content},
        {"timestamp", std::time(nullptr)},
        {"sent", true}
    };

    send_json_response(res, 201, response);
    LOG_INFO("Message sent from " + auth_result.username + " to " + to_user);
}

void MessageHandlers::handle_get_conversations(const httplib::Request& req, httplib::Response& res) {
    auto auth_result = AuthMiddleware::validate_token(req);
    if (!auth_result.is_valid) {
        send_error_response(res, 401, auth_result.error_message);
        return;
    }

    auto conversations = message_manager_->get_user_conversations(auth_result.username);
    json conversations_array = json::array();

    for (const auto& conversation : conversations) {
        conversations_array.push_back(message_manager_->conversation_to_json(conversation, false));
    }

    json response = {
        {"conversations", conversations_array},
        {"total", conversations_array.size()}
    };

    send_json_response(res, 200, response);
    LOG_INFO("Conversations retrieved for user: " + auth_result.username + " (" + std::to_string(conversations.size()) + " conversations)");
}

void MessageHandlers::handle_get_messages(const httplib::Request& req, httplib::Response& res) {
    auto auth_result = AuthMiddleware::validate_token(req);
    if (!auth_result.is_valid) {
        send_error_response(res, 401, auth_result.error_message);
        return;
    }

    std::string conv_id = req.matches[1];

    auto messages = message_manager_->get_conversation_messages(conv_id, auth_result.username);
    if (messages.empty()) {
        // Check if conversation exists but user has no access
        auto conversation = message_manager_->get_conversation(conv_id, auth_result.username);
        if (!conversation.has_value()) {
            send_error_response(res, 404, "Conversation not found or access denied");
            return;
        }
    }

    json messages_array = json::array();
    for (const auto& message : messages) {
        messages_array.push_back(message_manager_->message_to_json(message));
    }

    json response = {
        {"conversation_id", conv_id},
        {"messages", messages_array},
        {"total", messages_array.size()}
    };

    send_json_response(res, 200, response);
    LOG_INFO("Messages retrieved for conversation: " + conv_id + " by user: " + auth_result.username + " (" + std::to_string(messages.size()) + " messages)");
}

void MessageHandlers::handle_mark_as_read(const httplib::Request& req, httplib::Response& res) {
    auto auth_result = AuthMiddleware::validate_token(req);
    if (!auth_result.is_valid) {
        send_error_response(res, 401, auth_result.error_message);
        return;
    }

    std::string message_id = req.matches[1];

    if (!message_manager_->mark_message_as_read(message_id, auth_result.username)) {
        send_error_response(res, 404, "Message not found or access denied");
        return;
    }

    json response = {
        {"message_id", message_id},
        {"marked_as_read", true}
    };

    send_json_response(res, 200, response);
    LOG_INFO("Message marked as read: " + message_id + " by user: " + auth_result.username);
}

void MessageHandlers::handle_delete_message(const httplib::Request& req, httplib::Response& res) {
    auto auth_result = AuthMiddleware::validate_token(req);
    if (!auth_result.is_valid) {
        send_error_response(res, 401, auth_result.error_message);
        return;
    }

    std::string message_id = req.matches[1];

    if (!message_manager_->delete_message(message_id, auth_result.username)) {
        send_error_response(res, 404, "Message not found or access denied");
        return;
    }

    json response = {
        {"message_id", message_id},
        {"deleted", true}
    };

    send_json_response(res, 200, response);
    LOG_INFO("Message deleted: " + message_id + " by user: " + auth_result.username);
}

bool MessageHandlers::validate_message_content(const std::string& content, std::string& error_message) {
    if (content.empty()) {
        error_message = "Message content cannot be empty";
        return false;
    }

    if (content.length() > 1000) {
        error_message = "Message content must be 1000 characters or less";
        return false;
    }

    // Check for only whitespace
    if (content.find_first_not_of(" \t\n\r") == std::string::npos) {
        error_message = "Message content cannot be only whitespace";
        return false;
    }

    return true;
}

void MessageHandlers::send_json_response(httplib::Response& res, int status, const json& data) {
    res.status = status;
    res.set_content(data.dump(2), "application/json");
}

void MessageHandlers::send_error_response(httplib::Response& res, int status, const std::string& message) {
    const json error_data = {
        {"error", true},
        {"message", message},
        {"status", status}
    };
    send_json_response(res, status, error_data);
}