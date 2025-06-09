#pragma once
#include <string>
#include <memory>
#include <atomic>
#include <thread>

class ServiceBase {
public:
    explicit ServiceBase(const std::string& service_name, int port);
    virtual ~ServiceBase() = default;

    virtual void start();

    virtual void stop();

    bool is_running() const;

    const std::string& get_name() const;

    int get_port() const;

protected:
    virtual void on_start() = 0;
    virtual void on_stop() = 0;
    virtual void run_service() = 0;

private:
    std::string service_name_;
    int port_;
    std::atomic<bool> is_running_;
    std::unique_ptr<std::thread> service_thread_;
};
