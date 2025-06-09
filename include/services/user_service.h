#pragma once
#include "common/http_service.h"
#include <unordered_map>
#include <mutex>

struct User {
    std::string username;
    std::string email;
    std::string full_name;
    bool is_online;
    std::time_t last_seen;
    std::time_t created_at;
};

class UserService final : public HttpService {
public:
    explicit UserService(int port = 8002);

protected:
    void setup_routes() override;

private:
    // Route handlers - НЕ static!
    void handle_get_user(const httplib::Request& req, httplib::Response& res);
    void handle_update_user(const httplib::Request& req, httplib::Response& res);
    void handle_get_users(const httplib::Request& req, httplib::Response& res);
    void handle_search_users(const httplib::Request& req, httplib::Response& res);
    void handle_set_online_status(const httplib::Request& req, httplib::Response& res);

    // Helper methods - НЕ static!
    bool user_exists(const std::string& username);
    json user_to_json(const User& user);
    bool validate_auth_token(const httplib::Request& req, std::string& username);
    void create_sample_users();

    // In-memory storage (in real app would be database)
    std::unordered_map<std::string, User> users_;
    mutable std::mutex users_mutex_;
};