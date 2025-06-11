#include "common/request_validator.h"
#include "common/logger.h"

const std::regex RequestValidator::email_regex_(
    R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)"
);

RequestValidator::ValidationResult RequestValidator::validate_json_body(
    const httplib::Request& req,
    const std::vector<std::string>& required_fields) {

    ValidationResult result;
    result.is_valid = false;

    if (req.body.empty()) {
        result.error_message = "Request body is empty";
        return result;
    }

    try {
        result.data = json::parse(req.body);
    } catch (const json::exception& e) {
        result.error_message = "Invalid JSON format: " + std::string(e.what());
        return result;
    }

    for (const auto& field : required_fields) {
        if (!result.data.contains(field)) {
            result.error_message = "Missing required field: " + field;
            return result;
        }
    }

    result.is_valid = true;
    return result;
}

RequestValidator::ValidationResult RequestValidator::validate_query_params(
    const httplib::Request& req,
    const std::vector<std::string>& required_params) {

    ValidationResult result;
    result.is_valid = false;
    result.data = json::object();

    for (const auto& param : required_params) {
        std::string value = req.get_param_value(param);
        if (value.empty()) {
            result.error_message = "Missing required parameter: " + param;
            return result;
        }
        result.data[param] = value;
    }

    result.is_valid = true;
    return result;
}

bool RequestValidator::is_valid_email(const std::string& email) {
    return std::regex_match(email, email_regex_);
}

bool RequestValidator::is_valid_username(const std::string& username) {
    return username.length() >= 3 && username.length() <= 50 &&
           std::regex_match(username, std::regex(R"(^[a-zA-Z0-9_]+$)"));
}