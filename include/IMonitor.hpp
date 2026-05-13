#pragma once
#include <string>
#include <functional>
#include <string_view>

class IMonitor {
public:
    virtual ~IMonitor() = default;

    virtual void setEventCallback(std::function<void(std::string_view, std::string_view)> callback) = 0;
    virtual void addWatchPart(const std::string& path) = 0;
    virtual void start() = 0;
    virtual void stop()  = 0;
};