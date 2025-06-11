#pragma once

#include <nlohmann/json.hpp>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <mutex>
#include <vector>

using json = nlohmann::json;

struct WebSocketConnection {
    std::string user_id;
    std::string connection_id;
    std::time_t connected_at;
    std::time_t last_activity;
    bool is_active;
};

class ConnectionManager {
public:
    ConnectionManager();

    // Connection operations
    std::string add_connection(const std::string& user_id);
    bool remove_connection(const std::string& connection_id);
    bool remove_user_connections(const std::string& user_id);
    void cleanup_inactive_connections();

    // Queries
    std::vector<std::string> get_online_users();
    size_t get_total_connections();
    size_t get_active_users_count();
    bool is_user_online(const std::string& user_id);

    // Utility
    json get_stats();

private:
    std::string generate_connection_id();

    std::unordered_map<std::string, WebSocketConnection> connections_;
    std::unordered_map<std::string, std::unordered_set<std::string>> user_connections_;
    std::mutex connections_mutex_;
    size_t connection_counter_;
};