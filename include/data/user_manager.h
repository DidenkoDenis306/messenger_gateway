#pragma once

#include <nlohmann/json.hpp>
#include <unordered_map>
#include <string>
#include <mutex>
#include <vector>
#include <optional>

using json = nlohmann::json;

struct User {
    std::string username;
    std::string email;
    std::string full_name;
    bool is_online;
    std::time_t last_seen;
    std::time_t created_at;
};

class UserManager {
public:
    UserManager();

    // User operations
    bool user_exists(const std::string& username);
    bool add_user(const User& user);
    bool update_user(const std::string& username, const json& updates);
    bool set_online_status(const std::string& username, bool is_online);

    // Queries
    std::optional<User> get_user(const std::string& username);
    std::vector<User> get_all_users(const std::string& exclude_username = "");
    std::vector<User> search_users(const std::string& query, const std::string& exclude_username = "");

    // Utility
    json user_to_json(const User& user);

private:
    void create_sample_users();

    std::unordered_map<std::string, User> users_;
    std::mutex users_mutex_;
};