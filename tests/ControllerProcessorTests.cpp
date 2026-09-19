#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>
#include "../src/controller/ControllerProcessor.h"
#include "../src/diagnostics/Statistics.h"

namespace GamepadReceiverTests {

void runControllerProcessorTests() {
    std::cout << "[Test] Running ControllerProcessorTests...\n";

    GamepadReceiver::Statistics stats;
    GamepadReceiver::ControllerProcessor processor(stats);

    // 1. Initial Packet
    GamepadReceiver::ControllerState s1{};
    s1.sequence = 100;
    s1.buttons = GamepadReceiver::BTN_A;
    assert(processor.processPacket(s1) == true);
    assert(processor.getLatestState().isButtonPressed(GamepadReceiver::BTN_A));

    // 2. Out-of-order packet (seq 99) should be rejected
    GamepadReceiver::ControllerState sOld{};
    sOld.sequence = 99;
    assert(processor.processPacket(sOld) == false);
    assert(processor.getLatestState().sequence == 100); // Sequence remains 100

    // 3. Newer packet with drop (seq 103, packets 101 and 102 lost)
    GamepadReceiver::ControllerState sNew{};
    sNew.sequence = 103;
    sNew.buttons = GamepadReceiver::BTN_B;
    assert(processor.processPacket(sNew) == true);
    assert(processor.getLatestState().sequence == 103);
    assert(processor.getLatestState().isButtonPressed(GamepadReceiver::BTN_B));
    assert(stats.getSnapshot().packetsLost == 2);

    // 4. Failsafe Test: Android app disconnects while button is held
    // Wait for 350ms (> 300ms watchdog timeout)
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    bool triggered = processor.checkFailsafe(300);
    assert(triggered == true);
    assert(processor.getLatestState().isNeutral() == true);
    assert(processor.getLatestState().buttons == 0);

    std::cout << "  -> ControllerProcessorTests PASSED!\n";
}

} // namespace GamepadReceiverTests
