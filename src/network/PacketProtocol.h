#pragma once

#include <cstdint>
#include <cstddef>

namespace GamepadReceiver {

// Binary Packet Constants
inline constexpr uint16_t PROTOCOL_MAGIC = 0x4750; // 'G' (0x47), 'P' (0x50) in ASCII
inline constexpr uint8_t  PROTOCOL_VERSION = 0x01;

inline constexpr uint8_t  PACKET_TYPE_STATE     = 0x01;
inline constexpr uint8_t  PACKET_TYPE_HELLO     = 0x02;
inline constexpr uint8_t  PACKET_TYPE_ACK       = 0x03;
inline constexpr uint8_t  PACKET_TYPE_HEARTBEAT = 0x04;

inline constexpr size_t CONTROLLER_PACKET_SIZE = 33;
inline constexpr size_t HANDSHAKE_PACKET_SIZE  = 12;

// Digital Button Flags (32-bit bitmask)
enum ButtonMask : uint32_t {
    BTN_NONE   = 0,
    BTN_A      = 1u << 0,
    BTN_B      = 1u << 1,
    BTN_X      = 1u << 2,
    BTN_Y      = 1u << 3,
    BTN_L1     = 1u << 4, // Left Bumper (LB)
    BTN_R1     = 1u << 5, // Right Bumper (RB)
    BTN_L3     = 1u << 6, // Left Thumb Click
    BTN_R3     = 1u << 7, // Right Thumb Click
    BTN_START  = 1u << 8, // Start / Menu
    BTN_SELECT = 1u << 9  // Select / Back
};

// D-Pad Flags (8-bit bitmask)
enum DpadMask : uint8_t {
    DPAD_CENTER = 0,
    DPAD_UP     = 1u << 0,
    DPAD_DOWN   = 1u << 1,
    DPAD_LEFT   = 1u << 2,
    DPAD_RIGHT  = 1u << 3
};

} // namespace GamepadReceiver
