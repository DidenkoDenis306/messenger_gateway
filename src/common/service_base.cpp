#include "common/service_base.h"
#include "common/logger.h"

ServiceBase::ServiceBase(const std::string& service_name, int port)
    : service_name_(service_name), port_(port), is_running_(false) {
}

void ServiceBase::start() {
    if (is_running_) {
        LOG_WARNING(service_name_ + " is already running");
        return;
    }

    LOG_INFO("Service " + service_name_ + " started on port " + std::to_string(port_));

    is_running_ = true;
    on_start();

    // Start service in separate thread
    service_thread_ = std::make_unique<std::thread>(&ServiceBase::run_service, this);

    LOG_INFO(service_name_ + " successfully started!");
}

void ServiceBase::stop() {
    if (!is_running_) {
        LOG_WARNING(service_name_ + " is not running!");
        return;
    }

    LOG_INFO("Stopping service " + service_name_);

    is_running_ = false;
    on_stop();

    // Awaiting thread finishing
    if (service_thread_ && service_thread_->joinable()) {
        service_thread_->join();
    }

    LOG_INFO(service_name_ + " successfully stopped!");
}

bool ServiceBase::is_running() const {
    return is_running_;
}

const std::string& ServiceBase::get_name() const {
    return service_name_;
}

int ServiceBase::get_port() const {
    return port_;
}

