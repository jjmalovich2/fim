#pragma once
#include "IAlert.hpp"
#include <fstream>
#include <string>

class Alert : public IAlert {
private:
    std::ofstream m_logFile;

public:
    explicit Alert(const std::string& logFilePath);
    ~Alert() override;

    void logEvent(AlertSeverity severity,
                  std::string_view filepath,
                  std::string_view eventType,
                  std::string_view details) override;
};