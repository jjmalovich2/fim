#pragma once
#include "IBaseline.hpp"
#include <unordered_map>
#include <string>

class Baseline : public IBaseline {
private:
    std::unordered_map<std::string, std::string> m_state;
    std::string m_dbpath;

public:
    explicit Baseline(std::string dbPath = "baseline.json");
    ~Baseline() override = default;

    bool load() override;
    bool save() override;
    void updateHash(std::string_view filepath, std::string_view hash) override;
    std::optional<std::string> getHash(std::string_view filepath) const override;
    bool exists(std::string_view filepath) const override;
};