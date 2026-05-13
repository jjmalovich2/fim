#include "Alert.hpp"
#include "json.hpp"
#include <iostream>
#include <chrono>

using json = nlohmann::json;

static std::string severityToString(AlertSeverity severity) {
    switch (severity) {
        case AlertSeverity::INFO:     return "INFO";
        case AlertSeverity::LOW:      return "LOW";
        case AlertSeverity::MEDIUM:   return "MEDIUM";
        case AlertSeverity::CRITICAL: return "CRITICAL";
        default:                      return "UNKNOWN";
    }
}

Alert::Alert(const std::string& logFilePath) {
    m_logFile.open(logFilePath, std::ios::app);
    if (!m_logFile.is_open()) {
        std::cerr << "Warn: failed to open log file: " << logFilePath << "\n";
    }
}

Alert::~Alert() {
    if (m_logFile.is_open()) m_logFile.close();
}

void Alert::logEvent(AlertSeverity severity, 
                     std::string_view filepath, 
                     std::string_view eventType, 
                     std::string_view details) {
    json alert;
    
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    alert["timestamp"] = timestamp;
    alert["severity"] = severityToString(severity);
    alert["file"] = filepath;
    alert["event_type"] = eventType;
    alert["details"] = details;

    std::string log_line = alert.dump();

    std::cout << log_line << "\n";

    if (m_logFile.is_open()) {
        m_logFile << log_line << "\n";
        m_logFile.flush(); // force write to disk lowk
    }
}