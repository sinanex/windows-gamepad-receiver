#pragma once

#include <cstdint>
#include "../controller/ControllerState.h"

namespace GamepadReceiver {

// Standard Windows XInput Gamepad representation
struct XInputReport {
    uint16_t wButtons{0};
    uint8_t  bLeftTrigger{0};
    uint8_t  bRightTrigger{0};
    int16_t  sThumbLX{0};
    int16_t  sThumbLY{0};
    int16_t  sThumbRX{0};
    int16_t  sThumbRY{0};
};

class GamepadMapper {
public:
    static XInputReport mapToXInput(const ControllerState& state, bool invertY = false) noexcept;
};

} // namespace GamepadReceiver
