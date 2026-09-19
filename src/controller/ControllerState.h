#pragma once

#include <cstdint>
#include "../network/PacketProtocol.h"

namespace GamepadReceiver {

struct ControllerState {
    int16_t  leftX{0};
    int16_t  leftY{0};
    int16_t  rightX{0};
    int16_t  rightY{0};
    uint16_t leftTrigger{0};
    uint16_t rightTrigger{0};
    uint32_t buttons{0};
    uint8_t  dpad{0};
    int64_t  timestampNs{0};
    uint32_t sequence{0};

    bool isButtonPressed(ButtonMask mask) const noexcept {
        return (buttons & static_cast<uint32_t>(mask)) != 0;
    }

    bool isDpad(DpadMask mask) const noexcept {
        return (dpad & static_cast<uint8_t>(mask)) != 0;
    }

    void resetToNeutral() noexcept {
        leftX = 0;
        leftY = 0;
        rightX = 0;
        rightY = 0;
        leftTrigger = 0;
        rightTrigger = 0;
        buttons = 0;
        dpad = 0;
    }

    bool isNeutral() const noexcept {
        return leftX == 0 && leftY == 0 &&
               rightX == 0 && rightY == 0 &&
               leftTrigger == 0 && rightTrigger == 0 &&
               buttons == 0 && dpad == 0;
    }
};

} // namespace GamepadReceiver
