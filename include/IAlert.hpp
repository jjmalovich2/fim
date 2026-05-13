#pragma once
#include <string_view>

enum class AlertSeverity {
    INFO,
    LOW,
    MEDIUM,
    CRITICAL
};

class IAlert {
public:
    virtual ~IAlert() = default;

    virtual void logEvent(AlertSeverity severity,
                          std::string_view filepath,
                          std::string_view eventType,
                          std::string_view details) = 0;
};