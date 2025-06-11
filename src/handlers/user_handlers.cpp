#include "handlers/user_handlers.h"
#include "common/auth_middleware.h"
#include "common/request_validator.h"
#include "common/logger.h"

UserHandlers::UserHandlers(std::shared_ptr<UserManager> user_manager)
    : user_manager_(user_manager) {
}

void UserHandlers::handle_get_user(const httplib::Request& req, httplib::Response& res) {
    auto auth_result = AuthMiddleware::validate_token(req);
    if (!auth_result.is_valid) {
        send_error_response(res, 401, auth_result.error_message);
        return;
    }

    auto user = user_manager_->get_user(auth_result.username);
    if (!user.has_value()) {
        send_error_response(res, 404, "User not found");
        return;
    }

    json response = user_manager_->user_to_json(user.value());
    send_json_response(res, 200, response);
    LOG_INFO("Profile retrieved for user: " + auth_result.username);
}

void UserHandlers::handle_update_user(const httplib::Request& req, httplib::Response& res) {
    auto auth_result = AuthMiddleware::validate_token(req);
    if (!auth_result.is_valid) {
        send_error_response(res, 401, auth_result.error_message);
        return;
    }

    auto validation = RequestValidator::validate_json_body(req, {});
    if (!validation.is_valid) {
        send_error_response(res, 400, validation.error_message);
        return;
    }

    if (!user_manager_->update_user(auth_result.username, validation.data)) {
        send_error_response(res, 404, "User not found");
        return;
    }

    auto updated_user = user_manager_->get_user(auth_result.username);
    json response = user_manager_->user_to_json(updated_user.value());
    response["updated"] = true;

    send_json_response(res, 200, response);
    LOG_INFO("Profile updated for user: " + auth_result.username);
}

void UserHandlers::handle_get_users(const httplib::Request& req, httplib::Response& res) {
    auto auth_result = AuthMiddleware::validate_token(req);
    if (!auth_result.is_valid) {
        send_error_response(res, 401, auth_result.error_message);
        return;
    }

    auto users = user_manager_->get_all_users(auth_result.username);
    json users_array = json::array();

    for (const auto& user : users) {
        json user_info = {
            {"username", user.username},
            {"full_name", user.full_name},
            {"is_online", user.is_online},
            {"last_seen", user.last_seen}
        };
        users_array.push_back(user_info);
    }

    json response = {
        {"users", users_array},
        {"total", users_array.size()}
    };

    send_json_response(res, 200, response);
    LOG_INFO("Users list retrieved for: " + auth_result.username);
}

void UserHandlers::handle_search_users(const httplib::Request& req, httplib::Response& res) {
    auto auth_result = AuthMiddleware::validate_token(req);
    if (!auth_result.is_valid) {
        send_error_response(res, 401, auth_result.error_message);
        return;
    }

    std::string query = req.get_param_value("q");
    if (query.empty()) {
        send_error_response(res, 400, "Search query parameter 'q' is required");
        return;
    }

    auto users = user_manager_->search_users(query, auth_result.username);
    json results = json::array();

    for (const auto& user : users) {
        json user_info = {
            {"username", user.username},
            {"full_name", user.full_name},
            {"is_online", user.is_online}
        };
        results.push_back(user_info);
    }

    json response = {
        {"results", results},
        {"query", query},
        {"total", results.size()}
    };

    send_json_response(res, 200, response);
    LOG_INFO("User search performed: " + query + " (" + std::to_string(results.size()) + " results)");
}

void UserHandlers::handle_set_online_status(const httplib::Request& req, httplib::Response& res) {
    auto auth_result = AuthMiddleware::validate_token(req);
    if (!auth_result.is_valid) {
        send_error_response(res, 401, auth_result.error_message);
        return;
    }

    auto validation = RequestValidator::validate_json_body(req, {"is_online"});
    if (!validation.is_valid) {
        send_error_response(res, 400, validation.error_message);
        return;
    }

    bool is_online = validation.data["is_online"];

    if (!user_manager_->set_online_status(auth_result.username, is_online)) {
        send_error_response(res, 404, "User not found");
        return;
    }

    json response = {
        {"username", auth_result.username},
        {"is_online", is_online},
        {"updated", true}
    };

    send_json_response(res, 200, response);
    LOG_INFO("Online status updated for user: " + auth_result.username + " -> " + (is_online ? "online" : "offline"));
}

void UserHandlers::send_json_response(httplib::Response& res, int status, const json& data) {
    res.status = status;
    res.set_content(data.dump(2), "application/json");
}

void UserHandlers::send_error_response(httplib::Response& res, int status, const std::string& message) {
    const json error_data = {
        {"error", true},
        {"message", message},
        {"status", status}
    };
    send_json_response(res, status, error_data);
}