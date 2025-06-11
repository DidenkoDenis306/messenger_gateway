#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <regex>

using json = nlohmann::json;

class RequestValidator {
public:
    struct ValidationResult {
        bool is_valid;
        std::string error_message;
        json data;
    };

    static ValidationResult validate_json_body(const httplib::Request& req,
                                               const std::vector<std::string>& required_fields);

    static ValidationResult validate_query_params(const httplib::Request& req,
                                                   const std::vector<std::string>& required_params);

    static bool is_valid_email(const std::string& email);
    static bool is_valid_username(const std::string& username);

private:
    static const std::regex email_regex_;
};