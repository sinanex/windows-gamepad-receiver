#pragma once

#include <cstdint>
#include <cstddef>
#include "PacketProtocol.h"
#include "../controller/ControllerState.h"

namespace GamepadReceiver {

class PacketDecoder {
public:
    enum class DecodeResult {
        ValidControllerState,
        ValidHandshakeHello,
        ValidHandshakeAck,
        ValidHeartbeat,
        InvalidMagic,
        InvalidVersion,
        InvalidSize,
        InvalidType,
        InvalidData
    };

    /**
     * Explicit little-endian safe packet parser.
     * Guaranteed zero-copy and independent of compiler struct alignment.
     */
    static DecodeResult decode(
        const uint8_t* data,
        size_t length,
        ControllerState& outState,
        uint32_t& outHandshakeSeq,
        uint32_t& outClientId
    ) noexcept;

    static uint16_t readUint16LE(const uint8_t* p) noexcept {
        return static_cast<uint16_t>(p[0]) |
               (static_cast<uint16_t>(p[1]) << 8);
    }

    static int16_t readInt16LE(const uint8_t* p) noexcept {
        return static_cast<int16_t>(readUint16LE(p));
    }

    static uint32_t readUint32LE(const uint8_t* p) noexcept {
        return static_cast<uint32_t>(p[0]) |
               (static_cast<uint32_t>(p[1]) << 8) |
               (static_cast<uint32_t>(p[2]) << 16) |
               (static_cast<uint32_t>(p[3]) << 24);
    }

    static int64_t readInt64LE(const uint8_t* p) noexcept {
        uint64_t val = 0;
        for (int i = 0; i < 8; ++i) {
            val |= (static_cast<uint64_t>(p[i]) << (i * 8));
        }
        return static_cast<int64_t>(val);
    }
};

} // namespace GamepadReceiver
