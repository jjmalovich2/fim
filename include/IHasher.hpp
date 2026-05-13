#pragma once
#include <string>
#include <string_view>

class IHasher {
public:
    virtual ~IHasher() = default;

    virtual std::string computeFileHash(std::string_view filepath) = 0;

    virtual std::string_view getAlgorithmName() const = 0;
};