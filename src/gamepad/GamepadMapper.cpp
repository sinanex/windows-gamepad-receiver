#include "GamepadMapper.h"

namespace GamepadReceiver {

// Standard XInput definitions without requiring <Xinput.h> header conflicts
constexpr uint16_t XINPUT_BTN_DPAD_UP        = 0x0001;
constexpr uint16_t XINPUT_BTN_DPAD_DOWN      = 0x0002;
constexpr uint16_t XINPUT_BTN_DPAD_LEFT      = 0x0004;
constexpr uint16_t XINPUT_BTN_DPAD_RIGHT     = 0x0008;
constexpr uint16_t XINPUT_BTN_START          = 0x0010;
constexpr uint16_t XINPUT_BTN_BACK           = 0x0020;
constexpr uint16_t XINPUT_BTN_LEFT_THUMB     = 0x0040;
constexpr uint16_t XINPUT_BTN_RIGHT_THUMB    = 0x0080;
constexpr uint16_t XINPUT_BTN_LEFT_SHOULDER  = 0x0100;
constexpr uint16_t XINPUT_BTN_RIGHT_SHOULDER = 0x0200;
constexpr uint16_t XINPUT_BTN_A              = 0x1000;
constexpr uint16_t XINPUT_BTN_B              = 0x2000;
constexpr uint16_t XINPUT_BTN_X              = 0x4000;
constexpr uint16_t XINPUT_BTN_Y              = 0x8000;

XInputReport GamepadMapper::mapToXInput(const ControllerState& state, bool invertY) noexcept {
    XInputReport report{};

    // Thumbsticks (-32767 to 32767)
    report.sThumbLX = state.leftX;
    report.sThumbLY = invertY ? static_cast<int16_t>(-state.leftY) : state.leftY;

    report.sThumbRX = state.rightX;
    report.sThumbRY = invertY ? static_cast<int16_t>(-state.rightY) : state.rightY;

    // Triggers: protocol is 0..65535, XInput is 0..255
    report.bLeftTrigger  = static_cast<uint8_t>((static_cast<uint32_t>(state.leftTrigger) * 255u) / 65535u);
    report.bRightTrigger = static_cast<uint8_t>((static_cast<uint32_t>(state.rightTrigger) * 255u) / 65535u);

    // Digital Buttons
    uint16_t btns = 0;
    if (state.isButtonPressed(BTN_A))      btns |= XINPUT_BTN_A;
    if (state.isButtonPressed(BTN_B))      btns |= XINPUT_BTN_B;
    if (state.isButtonPressed(BTN_X))      btns |= XINPUT_BTN_X;
    if (state.isButtonPressed(BTN_Y))      btns |= XINPUT_BTN_Y;
    if (state.isButtonPressed(BTN_L1))     btns |= XINPUT_BTN_LEFT_SHOULDER;
    if (state.isButtonPressed(BTN_R1))     btns |= XINPUT_BTN_RIGHT_SHOULDER;
    if (state.isButtonPressed(BTN_L3))     btns |= XINPUT_BTN_LEFT_THUMB;
    if (state.isButtonPressed(BTN_R3))     btns |= XINPUT_BTN_RIGHT_THUMB;
    if (state.isButtonPressed(BTN_START))  btns |= XINPUT_BTN_START;
    if (state.isButtonPressed(BTN_SELECT)) btns |= XINPUT_BTN_BACK;

    // D-Pad
    if (state.isDpad(DPAD_UP))    btns |= XINPUT_BTN_DPAD_UP;
    if (state.isDpad(DPAD_DOWN))  btns |= XINPUT_BTN_DPAD_DOWN;
    if (state.isDpad(DPAD_LEFT))  btns |= XINPUT_BTN_DPAD_LEFT;
    if (state.isDpad(DPAD_RIGHT)) btns |= XINPUT_BTN_DPAD_RIGHT;

    report.wButtons = btns;
    return report;
}

} // namespace GamepadReceiver
