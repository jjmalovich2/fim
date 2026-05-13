#include "Blake3Hasher.hpp"
#include "blake3.h"

#include <fcntl.h>     // for open()
#include <unistd.h>    // for close()
#include <sys/mman.h>  // for mmap(), munmap()
#include <sys/stat.h>  // for fstat()
#include <iostream>

std::string Blake3Hasher::computeFileHash(std::string_view filepath) {
    // open file desc
    std::string path_str(filepath);
    int fd = open(path_str.c_str(), O_RDONLY);
    if (fd < 0) return ""; // implement: log errno (permission denied)

    // get file size
    struct stat sb;
    if (fstat(fd, &sb) < 0) {
        close(fd);
        return "";
    }

    uint8_t output[BLAKE3_OUT_LEN];
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);

    // handle empty files (mmap fails with 0-byte files)
    if (sb.st_size > 0) {
        // map file into memory
        void* map = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (map == MAP_FAILED) {
            close(fd);
            return "";
        }

        // stream the mmap file into blake3
        blake3_hasher_update(&hasher, map, sb.st_size);

        // cleanup mmap
        munmap(map, sb.st_size);
    }

    // hash then close file
    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
    close(fd);

    // zero-alloc hex conversion
    std::string hex_str;
    hex_str.reserve(BLAKE3_OUT_LEN * 2);
    static const char* hex_digits = "0123456789abcdef";

    for (size_t i = 0; i < BLAKE3_OUT_LEN; ++i) {
        hex_str.push_back(hex_digits[output[i] >> 4]);
        hex_str.push_back(hex_digits[output[i] & 15]);
    }

    return hex_str;
}

std::string_view Blake3Hasher::getAlgorithmName() const {
    return "BLAKE3";
}