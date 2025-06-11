#include "data/connection_manager.h"
#include "common/logger.h"
#include <sstream>

ConnectionManager::ConnectionManager() : connection_counter_(0) {
}

std::string ConnectionManager::add_connection(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    std::string connection_id = generate_connection_id();
    std::time_t now = std::time(nullptr);

    WebSocketConnection connection = {
        user_id,
        connection_id,
        now,
        now,
        true
    };

    connections_[connection_id] = connection;
    user_connections_[user_id].insert(connection_id);

    LOG_INFO("Connection added: " + connection_id + " for user: " + user_id);
    return connection_id;
}

bool ConnectionManager::remove_connection(const std::string& connection_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    auto it = connections_.find(connection_id);
    if (it == connections_.end()) {
        return false;
    }

    std::string user_id = it->second.user_id;
    connections_.erase(it);

    // Remove from user connections
    auto user_it = user_connections_.find(user_id);
    if (user_it != user_connections_.end()) {
        user_it->second.erase(connection_id);
        if (user_it->second.empty()) {
            user_connections_.erase(user_it);
        }
    }

    LOG_INFO("Connection removed: " + connection_id + " for user: " + user_id);
    return true;
}

bool ConnectionManager::remove_user_connections(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    auto user_it = user_connections_.find(user_id);
    if (user_it == user_connections_.end()) {
        return false;
    }

    // Remove all connections for this user
    for (const std::string& connection_id : user_it->second) {
        connections_.erase(connection_id);
    }

    user_connections_.erase(user_it);
    LOG_INFO("All connections removed for user: " + user_id);
    return true;
}

void ConnectionManager::cleanup_inactive_connections() {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    std::time_t now = std::time(nullptr);
    std::vector<std::string> to_remove;

    for (const auto& [connection_id, connection] : connections_) {
        if (now - connection.last_activity > 300) { // 5 minutes timeout
            to_remove.push_back(connection_id);
        }
    }

    for (const std::string& connection_id : to_remove) {
        auto it = connections_.find(connection_id);
        if (it != connections_.end()) {
            std::string user_id = it->second.user_id;
            connections_.erase(it);

            auto user_it = user_connections_.find(user_id);
            if (user_it != user_connections_.end()) {
                user_it->second.erase(connection_id);
                if (user_it->second.empty()) {
                    user_connections_.erase(user_it);
                }
            }
        }
    }

    if (!to_remove.empty()) {
        LOG_INFO("Cleaned up " + std::to_string(to_remove.size()) + " inactive connections");
    }
}

std::vector<std::string> ConnectionManager::get_online_users() {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    std::vector<std::string> users;
    for (const auto& [user_id, connections] : user_connections_) {
        if (!connections.empty()) {
            users.push_back(user_id);
        }
    }

    return users;
}

size_t ConnectionManager::get_total_connections() {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    return connections_.size();
}

size_t ConnectionManager::get_active_users_count() {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    return user_connections_.size();
}

bool ConnectionManager::is_user_online(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = user_connections_.find(user_id);
    return it != user_connections_.end() && !it->second.empty();
}

json ConnectionManager::get_stats() {
    std::lock_guard<std::mutex> lock(connections_mutex_);

    return {
        {"total_connections", connections_.size()},
        {"active_users", user_connections_.size()},
        {"timestamp", std::time(nullptr)}
    };
}

std::string ConnectionManager::generate_connection_id() {
    std::ostringstream oss;
    oss << "conn_" << ++connection_counter_ << "_" << std::time(nullptr);
    return oss.str();
}