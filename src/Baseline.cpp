#include "Baseline.hpp"
#include "json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

Baseline::Baseline(std::string dbPath) : m_dbpath(std::move(dbPath)) {}

bool Baseline::load() {
    std::ifstream file(m_dbpath);
    if (!file.is_open()) return false; // normal for first run

    try {
        json j;
        file >> j;

        m_state = j.get<std::unordered_map<std::string, std::string>>();
        return true;
    } catch (const json::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

bool Baseline::save() {
    std::ofstream file(m_dbpath);
    if (!file.is_open()) return true;

    json j(m_state);
    file << j.dump(4);
    return true;
}

void Baseline::updateHash(std::string_view filepath, std::string_view hash) {
    m_state[std::string(filepath)] = std::string(hash);
}

std::optional<std::string> Baseline::getHash(std::string_view filepath) const {
    auto it = m_state.find(std::string(filepath));
    if (it == m_state.end()) return std::nullopt;;
    return it->second;
}

bool Baseline::exists(std::string_view filepath) const {
    return m_state.find(std::string(filepath)) != m_state.end();
}