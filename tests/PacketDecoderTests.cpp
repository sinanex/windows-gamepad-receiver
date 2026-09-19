#include <cassert>
#include <iostream>
#include <vector>
#include "../src/network/PacketProtocol.h"
#include "../src/network/PacketDecoder.h"

namespace GamepadReceiverTests {

void runPacketDecoderTests() {
    std::cout << "[Test] Running PacketDecoderTests...\n";

    // Build exact test vector matching Android PacketEncoderTest:
    // Magic: 0x4750 (LE: 0x50, 0x47)
    // Version: 0x01
    // Type: 0x01
    // Sequence: 100 (LE: 100, 0, 0, 0)
    // Timestamp: 1,000,000,000 ns
    // LeftX = 1.0 (32767) (LE: 0xFF, 0x7F)
    // LeftY = 0.0 (0)
    // RightX = -1.0 (-32767) (LE: 0x01, 0x80)
    // RightY = 0.5 (16383) (LE: 0xFF, 0x3F)
    // LeftTrigger = 0 (0x00, 0x00)
    // RightTrigger = 65535 (0xFF, 0xFF)
    // Buttons = BTN_A (1) (LE: 0x01, 0x00, 0x00, 0x00)
    // DPad = 0
    std::vector<uint8_t> packet(GamepadReceiver::CONTROLLER_PACKET_SIZE, 0);

    packet[0] = 0x50; packet[1] = 0x47; // Magic 0x4750
    packet[2] = 0x01;                   // Version 1
    packet[3] = 0x01;                   // Type State

    // Seq = 100
    packet[4] = 100; packet[5] = 0; packet[6] = 0; packet[7] = 0;

    // Timestamp = 1000000000 = 0x3B9ACA00
    packet[8] = 0x00; packet[9] = 0xCA; packet[10] = 0x9A; packet[11] = 0x3B;

    // LeftX = 32767 (0x7FFF)
    packet[16] = 0xFF; packet[17] = 0x7F;
    // LeftY = 0
    packet[18] = 0x00; packet[19] = 0x00;

    // RightX = -32767 (0x8001)
    packet[20] = 0x01; packet[21] = 0x80;
    // RightY = 16383 (0x3FFF)
    packet[22] = 0xFF; packet[23] = 0x3F;

    // LeftTrigger = 0
    packet[24] = 0x00; packet[25] = 0x00;
    // RightTrigger = 65535 (0xFFFF)
    packet[26] = 0xFF; packet[27] = 0xFF;

    // Buttons = BTN_A (0x01)
    packet[28] = 0x01; packet[29] = 0x00; packet[30] = 0x00; packet[31] = 0x00;

    // DPad = 0
    packet[32] = 0x00;

    GamepadReceiver::ControllerState decodedState{};
    uint32_t hsSeq = 0, hsClient = 0;

    auto res = GamepadReceiver::PacketDecoder::decode(
        packet.data(), packet.size(), decodedState, hsSeq, hsClient
    );

    assert(res == GamepadReceiver::PacketDecoder::DecodeResult::ValidControllerState);
    assert(decodedState.sequence == 100);
    assert(decodedState.leftX == 32767);
    assert(decodedState.leftY == 0);
    assert(decodedState.rightX == -32767);
    assert(decodedState.rightY == 16383);
    assert(decodedState.leftTrigger == 0);
    assert(decodedState.rightTrigger == 65535);
    assert(decodedState.isButtonPressed(GamepadReceiver::BTN_A));
    assert(!decodedState.isButtonPressed(GamepadReceiver::BTN_B));
    assert(decodedState.dpad == 0);

    // Test rejection of corrupted magic
    packet[0] = 0x00;
    res = GamepadReceiver::PacketDecoder::decode(
        packet.data(), packet.size(), decodedState, hsSeq, hsClient
    );
    assert(res == GamepadReceiver::PacketDecoder::DecodeResult::InvalidMagic);

    // Test rejection of corrupted size
    res = GamepadReceiver::PacketDecoder::decode(
        packet.data(), 20, decodedState, hsSeq, hsClient
    );
    assert(res == GamepadReceiver::PacketDecoder::DecodeResult::InvalidSize);

    std::cout << "  -> PacketDecoderTests PASSED!\n";
}

} // namespace GamepadReceiverTests
