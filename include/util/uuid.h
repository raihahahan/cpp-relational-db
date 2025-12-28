#pragma once
#include <array>
#include <random>
#include <cstdint>
#include "config/config.h"

namespace util {

inline std::array<uint8_t, 16> GenerateUUID() {
    static thread_local std::mt19937_64 rng{std::random_device{}()};

    std::array<uint8_t, 16> uuid;
    for (auto& b : uuid) {
        b = static_cast<uint8_t>(rng());
    }

    // RFC 4122 version 4 (random)
    uuid[6] = (uuid[6] & 0x0F) | 0x40;
    uuid[8] = (uuid[8] & 0x3F) | 0x80;

    return uuid;
};

constexpr db::config::uuid_t MakeUUID(
    uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3,
    uint8_t b4, uint8_t b5, uint8_t b6, uint8_t b7,
    uint8_t b8, uint8_t b9, uint8_t b10, uint8_t b11,
    uint8_t b12, uint8_t b13, uint8_t b14, uint8_t b15
) {
    return db::config::uuid_t{{ b0, b1, b2, b3, b4, b5, b6, b7,
             b8, b9, b10, b11, b12, b13, b14, b15 }};
}

}
