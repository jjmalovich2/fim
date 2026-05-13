#pragma once
#include "IHasher.hpp"

class Blake3Hasher : public IHasher {
public:
    Blake3Hasher() = default;
    ~Blake3Hasher() override = default;

    std::string computeFileHash(std::string_view filepath) override;
    std::string_view getAlgorithmName() const override;
};