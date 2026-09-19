#include <iostream>

namespace GamepadReceiverTests {
    void runPacketDecoderTests();
    void runControllerProcessorTests();
    void runGamepadMappingTests();
}

int main() {
    std::cout << "====================================================\n";
    std::cout << "  Windows Virtual Gamepad Receiver Unit Test Suite   \n";
    std::cout << "====================================================\n\n";

    try {
        GamepadReceiverTests::runPacketDecoderTests();
        GamepadReceiverTests::runControllerProcessorTests();
        GamepadReceiverTests::runGamepadMappingTests();

        std::cout << "\n>>> ALL 3 TEST SUITES PASSED SUCCESSFULLY! <<<\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}
