cmake_minimum_required(VERSION 3.15)
project(WindowsGamepadReceiver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files for core engine
set(CORE_SOURCES
    src/network/PacketDecoder.cpp
    src/network/UdpReceiver.cpp
    src/controller/ControllerProcessor.cpp
    src/gamepad/GamepadMapper.cpp
    src/gamepad/ViGEmGamepad.cpp
    src/connection/ConnectionManager.cpp
    src/diagnostics/Statistics.cpp
    src/utils/SystemTray.cpp
    src/ui/MainWindow.cpp
)

# 1. Main Windows Gamepad Receiver Application
add_executable(WindowsGamepadReceiver WIN32
    src/main/main.cpp
    ${CORE_SOURCES}
)

target_link_libraries(WindowsGamepadReceiver PRIVATE
    ws2_32
    comctl32
    iphlpapi
)

# 2. Standalone Unit Tests Executable
add_executable(ReceiverUnitTests
    tests/TestRunner.cpp
    tests/PacketDecoderTests.cpp
    tests/ControllerProcessorTests.cpp
    tests/GamepadMappingTests.cpp
    src/network/PacketDecoder.cpp
    src/controller/ControllerProcessor.cpp
    src/diagnostics/Statistics.cpp
    src/gamepad/GamepadMapper.cpp
)

target_link_libraries(ReceiverUnitTests PRIVATE
    ws2_32
)

# 3. Local Test Packet Generator
add_executable(TestSender
    tests/TestSender.cpp
)

target_link_libraries(TestSender PRIVATE
    ws2_32
)
