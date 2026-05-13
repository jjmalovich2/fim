#include <iostream>
#include <filesystem>
#include "InotifyMonitor.hpp"
#include "Blake3Hasher.hpp"
#include "Baseline.hpp"
#include "Alert.hpp"

namespace fs = std::filesystem;

int main() {
    // init
    IHasher* hasher = new Blake3Hasher();
    IBaseline* baseline = new Baseline("fim_baseline.json");
    IAlert* alert = new Alert("logs.jsonl");
    IMonitor* monitor = new InotifyMonitor();

    std::string target_dir = "src"; // directory we want to protect

    // load baseline
    if (!baseline->load()) {
        alert->logEvent(AlertSeverity::INFO, "SYSTEM", "INIT", "No baseline found. Building initial state...");
        for (const auto& entry : fs::recursive_directory_iterator(target_dir)) {
            if (entry.is_regular_file()) {
                std::string path = entry.path().string();
                std::string hash = hasher->computeFileHash(path);
                if (!hash.empty()) {
                    baseline->updateHash(path, hash);
                }
            }
        }
        baseline->save();
        alert->logEvent(AlertSeverity::INFO, "SYSTEM", "READY", "Initial baseline built and saved.");
    } else {
        alert->logEvent(AlertSeverity::INFO, "SYSTEM", "READY", "Loaded existing baseline from disk.");
    }

    // fim logic
    monitor->setEventCallback([&](std::string_view filepath, std::string_view event_type) {
        // skip hashing if only a directory is modified
        if (fs::exists(filepath) && fs::is_directory(filepath)) return;

        if (event_type == "DELETED") {
            alert->logEvent(AlertSeverity::CRITICAL, filepath, event_type, "File was removed from disk.");
            return;
        }

        // compute hash
        std::string current_hash = hasher->computeFileHash(filepath);
        
        // stop if mmap fails
        if (current_hash.empty()) return; 

        // compare to baseline
        if (baseline->exists(filepath)) {
            std::string old_hash = baseline->getHash(filepath).value();
            
            // alert only if hash makes a significant change
            if (old_hash != current_hash) {
                std::string details = "Baseline: " + old_hash + " | Current: " + current_hash;
                alert->logEvent(AlertSeverity::CRITICAL, filepath, "MODIFIED", details);
                
                // update baseline
                baseline->updateHash(filepath, current_hash);
                baseline->save();
            }
        } else {
            alert->logEvent(AlertSeverity::LOW, filepath, "CREATED", "New file detected and hashed.");
            baseline->updateHash(filepath, current_hash);
            baseline->save();
        }
    });

    // start
    alert->logEvent(AlertSeverity::INFO, "SYSTEM", "START", "Monitoring started on directory: " + target_dir);
    monitor->addWatchPart(target_dir);
    
    // blocks forever
    monitor->start();

    // cleanup
    delete monitor;
    delete alert;
    delete baseline;
    delete hasher;
    return 0;
}