#include "InotifyMonitor.hpp"
#include <sys/inotify.h>
#include <unistd.h>
#include <filesystem>
#include <iostream>
#include <array>

namespace fs = std::filesystem;

InotifyMonitor::InotifyMonitor() : m_running(false) {
    m_fd = inotify_init();
    if (m_fd < 0) std::cout << "Fatal: failed to initialize inotify\n";
}

InotifyMonitor::~InotifyMonitor() {
    stop();
    if (m_fd >= 0) close(m_fd);
}

void InotifyMonitor::setEventCallback(std::function<void(std::string_view, std::string_view)> callback) {
    m_callback = std::move(callback);
}

void InotifyMonitor::addWatchPart(const std::string& path) {
    // lambda function to add a watch to a single dir
    auto add_watch = [this](const fs::path& p) {
        uint32_t mask = IN_MODIFY | IN_CREATE | IN_DELETE | IN_ATTRIB;
        int wd = inotify_add_watch(m_fd, p.c_str(), mask);
        if (wd >= 0) m_wd_to_path[wd] = p.string();
    };

    // add root target
    add_watch(path);

    // recursively add all subdirs
    if (fs::is_directory(path)) {
        for (auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.is_directory()) add_watch(entry.path());
        }
    }
}

void InotifyMonitor::start() {
    if (m_fd < 0 || !m_callback) return;
    m_running = true;

    // zero-alloc buffer on stack
    alignas(struct inotify_event) std::array<char, 4096> buffer;

    while (m_running) {
        // read call blocks the thread until kernel detects file change
        ssize_t len = read(m_fd, buffer.data(), buffer.size());
        if (len <= 0) {
            if (errno == EINTR) continue;
            break;
        }

        // iterate through raw bytes and cast to inotify_events
        // shoutout gemma 4 for this algorithm
        for (char* ptr = buffer.data(); ptr < buffer.data() + len;) {
            struct inotify_event* event = reinterpret_cast<struct inotify_event*>(ptr);

            if (event->len > 0) {
                auto it = m_wd_to_path.find(event->wd);
                if (it != m_wd_to_path.end()) {
                    std::string full_path = it->second + "/" + event->name;
                    
                    std::string_view event_type = "UNKNOWN";
                    if (event->mask & IN_MODIFY) event_type = "MODIFIED";
                    else if (event->mask & IN_CREATE) event_type = "CREATED";
                    else if (event->mask & IN_DELETE) event_type = "DELETED";
                    else if (event->mask & IN_ATTRIB) event_type = "ATTRIB_CHANGED";

                    // Fire the callback
                    m_callback(full_path, event_type);
                }
            }

            ptr += sizeof(struct inotify_event) + event->len;
        }
    }
}

void InotifyMonitor::stop() {
    m_running = false;
}