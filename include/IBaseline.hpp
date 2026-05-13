#pragma once
#include <string>
#include <string_view>
#include <optional>

class IBaseline {
public:
    virtual ~IBaseline() = default;

    virtual bool load() = 0;
    virtual bool save() = 0;
    virtual void updateHash(std::string_view filepath, std::string_view hash) = 0;
    virtual std::optional<std::string> getHash(std::string_view filepath) const = 0;
    virtual bool exists(std::string_view filepath) const = 0;
};