#pragma once
#include "IMonitor.hpp"
#include <unordered_map>
#include <atomic>

class InotifyMonitor : public IMonitor {
private:
    int m_fd;
    std::atomic<bool> m_running;
    std::unordered_map<int, std::string> m_wd_to_path;
    std::function<void(std::string_view, std::string_view)> m_callback;

public:
    InotifyMonitor();
    ~InotifyMonitor() override;

    void setEventCallback(std::function<void(std::string_view, std::string_view)> callback) override;
    void addWatchPart(const std::string& path) override;
    void start() override;
    void stop() override;
};