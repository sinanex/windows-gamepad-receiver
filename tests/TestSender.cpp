/**
 * Local UDP Test Packet Generator (TestSender)
 * Simulates an Android Virtual Gamepad sending 120Hz UDP packets to the Windows Receiver.
 * 
 * To compile:
 *   cl.exe /EHsc TestSender.cpp ws2_32.lib
 *   or g++ TestSender.cpp -lws2_32 -o TestSender.exe
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char* argv[]) {
    std::string ip = "127.0.0.1";
    int port = 5000;

    if (argc > 1) ip = argv[1];
    if (argc > 2) port = std::stoi(argv[2]);

    std::cout << "========================================================\n";
    std::cout << "  Virtual Gamepad Test Packet Generator (120Hz)         \n";
    std::cout << "  Streaming simulated input to " << ip << ":" << port << "\n";
    std::cout << "========================================================\n\n";

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &dest.sin_addr);

    // Handshake Hello (12 bytes)
    uint8_t hello[12] = {
        0x50, 0x47, // Magic
        0x01,       // Version
        0x02,       // Hello
        0x01, 0, 0, 0, // Seq
        0, 0, 0, 0     // ClientId
    };
    sendto(sock, (const char*)hello, sizeof(hello), 0, (sockaddr*)&dest, sizeof(dest));
    std::cout << "[Handshake] Sent Hello packet to receiver.\n";

    std::vector<uint8_t> pkt(33, 0);
    // Header
    pkt[0] = 0x50; pkt[1] = 0x47; // 0x4750
    pkt[2] = 0x01;               // Version 1
    pkt[3] = 0x01;               // State type

    uint32_t seq = 0;
    float angle = 0.0f;

    std::cout << "[Streaming] Sending 120Hz animated joystick circle & button pulses...\n";
    std::cout << "Press Ctrl+C to stop.\n";

    while (true) {
        seq++;
        angle += 0.05f;

        // Sequence
        pkt[4] = static_cast<uint8_t>(seq & 0xFF);
        pkt[5] = static_cast<uint8_t>((seq >> 8) & 0xFF);
        pkt[6] = static_cast<uint8_t>((seq >> 16) & 0xFF);
        pkt[7] = static_cast<uint8_t>((seq >> 24) & 0xFF);

        // Circular sweep on Left Thumbstick
        int16_t lx = static_cast<int16_t>(std::cos(angle) * 32000.0f);
        int16_t ly = static_cast<int16_t>(std::sin(angle) * 32000.0f);
        pkt[16] = static_cast<uint8_t>(lx & 0xFF);
        pkt[17] = static_cast<uint8_t>((lx >> 8) & 0xFF);
        pkt[18] = static_cast<uint8_t>(ly & 0xFF);
        pkt[19] = static_cast<uint8_t>((ly >> 8) & 0xFF);

        // Oscillating Right Trigger (RT)
        uint16_t rt = static_cast<uint16_t>((std::sin(angle * 2.0f) * 0.5f + 0.5f) * 65535.0f);
        pkt[26] = static_cast<uint8_t>(rt & 0xFF);
        pkt[27] = static_cast<uint8_t>((rt >> 8) & 0xFF);

        // Pulsing A and B buttons
        uint32_t btns = 0;
        if (std::sin(angle) > 0.3f) btns |= (1 << 0); // A
        if (std::cos(angle) > 0.3f) btns |= (1 << 1); // B
        pkt[28] = static_cast<uint8_t>(btns & 0xFF);

        sendto(sock, (const char*)pkt.data(), 33, 0, (sockaddr*)&dest, sizeof(dest));

        // Sleep ~8.33ms for 120Hz
        std::this_thread::sleep_for(std::chrono::microseconds(8333));
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
