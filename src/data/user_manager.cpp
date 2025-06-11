#include "data/user_manager.h"
#include "common/logger.h"
#include <algorithm>

UserManager::UserManager() {
    create_sample_users();
}

bool UserManager::user_exists(const std::string& username) {
    std::lock_guard<std::mutex> lock(users_mutex_);
    return users_.find(username) != users_.end();
}

bool UserManager::add_user(const User& user) {
    std::lock_guard<std::mutex> lock(users_mutex_);

    if (users_.find(user.username) != users_.end()) {
        return false; // User already exists
    }

    users_[user.username] = user;
    LOG_INFO("User added: " + user.username);
    return true;
}

bool UserManager::update_user(const std::string& username, const json& updates) {
    std::lock_guard<std::mutex> lock(users_mutex_);

    auto it = users_.find(username);
    if (it == users_.end()) {
        return false;
    }

    if (updates.contains("full_name")) {
        it->second.full_name = updates["full_name"];
    }
    if (updates.contains("email")) {
        it->second.email = updates["email"];
    }

    LOG_INFO("User updated: " + username);
    return true;
}

bool UserManager::set_online_status(const std::string& username, bool is_online) {
    std::lock_guard<std::mutex> lock(users_mutex_);

    auto it = users_.find(username);
    if (it == users_.end()) {
        return false;
    }

    it->second.is_online = is_online;
    it->second.last_seen = std::time(nullptr);

    LOG_INFO("User status updated: " + username + " -> " + (is_online ? "online" : "offline"));
    return true;
}

std::optional<User> UserManager::get_user(const std::string& username) {
    std::lock_guard<std::mutex> lock(users_mutex_);

    auto it = users_.find(username);
    if (it == users_.end()) {
        return std::nullopt;
    }

    return it->second;
}

std::vector<User> UserManager::get_all_users(const std::string& exclude_username) {
    std::lock_guard<std::mutex> lock(users_mutex_);

    std::vector<User> result;
    for (const auto& [username, user] : users_) {
        if (username != exclude_username) {
            result.push_back(user);
        }
    }

    return result;
}

std::vector<User> UserManager::search_users(const std::string& query, const std::string& exclude_username) {
    std::lock_guard<std::mutex> lock(users_mutex_);

    std::vector<User> result;
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);

    for (const auto& [username, user] : users_) {
        if (username == exclude_username) continue;

        std::string lower_username = user.username;
        std::string lower_fullname = user.full_name;
        std::transform(lower_username.begin(), lower_username.end(), lower_username.begin(), ::tolower);
        std::transform(lower_fullname.begin(), lower_fullname.end(), lower_fullname.begin(), ::tolower);

        if (lower_username.find(lower_query) != std::string::npos ||
            lower_fullname.find(lower_query) != std::string::npos) {
            result.push_back(user);
        }
    }

    return result;
}

json UserManager::user_to_json(const User& user) {
    return {
        {"username", user.username},
        {"email", user.email},
        {"full_name", user.full_name},
        {"is_online", user.is_online},
        {"last_seen", user.last_seen},
        {"created_at", user.created_at}
    };
}

void UserManager::create_sample_users() {
    std::time_t now = std::time(nullptr);

    users_["alice"] = {
        "alice", "alice@example.com", "Alice Johnson",
        true, now, now - 86400
    };

    users_["bob"] = {
        "bob", "bob@example.com", "Bob Smith",
        false, now - 3600, now - 172800
    };

    users_["charlie"] = {
        "charlie", "charlie@example.com", "Charlie Brown",
        true, now, now - 259200
    };

    LOG_INFO("Sample users created: alice, bob, charlie");
}