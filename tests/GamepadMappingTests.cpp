#include <cassert>
#include <iostream>
#include "../src/gamepad/GamepadMapper.h"

namespace GamepadReceiverTests {

void runGamepadMappingTests() {
    std::cout << "[Test] Running GamepadMappingTests...\n";

    GamepadReceiver::ControllerState state{};
    state.leftX = -32767;
    state.leftY = 32767;
    state.rightX = 0;
    state.rightY = -16384;
    state.leftTrigger = 0;
    state.rightTrigger = 65535; // 100% full press -> 255 in XInput
    state.buttons = GamepadReceiver::BTN_A | GamepadReceiver::BTN_R1 | GamepadReceiver::BTN_START;
    state.dpad = GamepadReceiver::DPAD_UP | GamepadReceiver::DPAD_RIGHT;

    auto xinput = GamepadReceiver::GamepadMapper::mapToXInput(state);

    assert(xinput.sThumbLX == -32767);
    assert(xinput.sThumbLY == 32767);
    assert(xinput.sThumbRX == 0);
    assert(xinput.sThumbRY == -16384);

    assert(xinput.bLeftTrigger == 0);
    assert(xinput.bRightTrigger == 255);

    // Verify XInput button flags
    assert((xinput.wButtons & 0x1000) != 0); // A
    assert((xinput.wButtons & 0x0200) != 0); // RB (Right Shoulder)
    assert((xinput.wButtons & 0x0010) != 0); // START
    assert((xinput.wButtons & 0x2000) == 0); // B not pressed

    // Verify D-Pad flags
    assert((xinput.wButtons & 0x0001) != 0); // DPAD_UP
    assert((xinput.wButtons & 0x0008) != 0); // DPAD_RIGHT
    assert((xinput.wButtons & 0x0002) == 0); // DPAD_DOWN not pressed

    std::cout << "  -> GamepadMappingTests PASSED!\n";
}

} // namespace GamepadReceiverTests
