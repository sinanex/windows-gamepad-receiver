#include "UdpReceiver.h"
#include "PacketDecoder.h"
#include "../utils/Logger.h"

#pragma comment(lib, "ws2_32.lib")

namespace GamepadReceiver {

UdpReceiver::UdpReceiver(
    ControllerProcessor& processor,
    VirtualGamepad* gamepad,
    Statistics& statistics
) : m_processor(processor),
    m_gamepad(gamepad),
    m_stats(statistics) {}

UdpReceiver::~UdpReceiver() {
    stop();
}

bool UdpReceiver::start(uint16_t port) {
    stop();

    m_port = port;
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LOG_ERROR("WSAStartup failed");
        m_state = ReceiverState::Error;
        return false;
    }

    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET) {
        LOG_ERROR("Failed to create UDP socket: " + std::to_string(WSAGetLastError()));
        WSACleanup();
        m_state = ReceiverState::Error;
        return false;
    }

    DWORD timeoutMs = 500;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));

    sockaddr_in serverAddr{};
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port        = htons(port);

    if (bind(m_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        LOG_ERROR("Failed to bind UDP socket to port " + std::to_string(port) + ": " + std::to_string(WSAGetLastError()));
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        WSACleanup();
        m_state = ReceiverState::Error;
        return false;
    }

    m_running = true;
    m_state = ReceiverState::Listening;
    if (m_statusCallback) m_statusCallback(m_state);

    m_receiverThread = std::thread(&UdpReceiver::receiverLoop, this);
    LOG_INFO("UDP Gamepad Receiver listening on 0.0.0.0:" + std::to_string(port));
    return true;
}

void UdpReceiver::stop() {
    if (m_running.exchange(false)) {
        if (m_socket != INVALID_SOCKET) {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
        }

        if (m_receiverThread.joinable()) {
            m_receiverThread.join();
        }

        WSACleanup();
        m_state = ReceiverState::Stopped;
        if (m_statusCallback) m_statusCallback(m_state);
        LOG_INFO("UDP Gamepad Receiver stopped");
    }
}

void UdpReceiver::receiverLoop() {
    uint8_t buffer[256];
    sockaddr_in clientAddr{};
    int clientAddrLen = sizeof(clientAddr);

    ControllerState state{};
    uint32_t hsSeq = 0;
    uint32_t hsClientId = 0;

    auto lastOneSecTick = std::chrono::steady_clock::now();

    while (m_running.load(std::memory_order_relaxed)) {
        int bytes = recvfrom(
            m_socket,
            (char*)buffer,
            sizeof(buffer),
            0,
            (sockaddr*)&clientAddr,
            &clientAddrLen
        );

        const auto now = std::chrono::steady_clock::now();

        if (now - lastOneSecTick >= std::chrono::seconds(1)) {
            m_stats.tickOneSecond();
            lastOneSecTick = now;
        }

        // Failsafe watchdog check (300ms)
        if (m_processor.checkFailsafe(300)) {
            if (m_gamepad) m_gamepad->reset();
            if (m_state == ReceiverState::Connected) {
                m_state = ReceiverState::Listening;
                if (m_statusCallback) m_statusCallback(m_state);
            }
        }

        if (bytes <= 0) {
            continue;
        }

        auto result = PacketDecoder::decode(buffer, bytes, state, hsSeq, hsClientId);

        switch (result) {
            case PacketDecoder::DecodeResult::ValidControllerState: {
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
                {
                    std::lock_guard<std::mutex> lock(m_clientMutex);
                    m_clientIp = ipStr;
                }
                m_clientPort = ntohs(clientAddr.sin_port);

                if (m_state != ReceiverState::Connected) {
                    m_state = ReceiverState::Connected;
                    if (m_statusCallback) m_statusCallback(m_state);
                }

                if (m_processor.processPacket(state)) {
                    if (m_gamepad && m_gamepad->update(state)) {
                        m_stats.onControllerUpdated();
                    } else {
                        m_stats.onGamepadError();
                    }
                }
                break;
            }

            case PacketDecoder::DecodeResult::ValidHandshakeHello: {
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
                {
                    std::lock_guard<std::mutex> lock(m_clientMutex);
                    m_clientIp = ipStr;
                }
                m_clientPort = ntohs(clientAddr.sin_port);

                m_state = ReceiverState::Connected;
                if (m_statusCallback) m_statusCallback(m_state);

                sendHandshakeResponse(m_socket, clientAddr, PACKET_TYPE_ACK, hsSeq);
                break;
            }

            case PacketDecoder::DecodeResult::ValidHeartbeat: {
                sendHandshakeResponse(m_socket, clientAddr, PACKET_TYPE_HEARTBEAT, hsSeq);
                break;
            }

            default:
                break;
        }
    }
}

void UdpReceiver::sendHandshakeResponse(
    SOCKET sock,
    const sockaddr_in& targetAddr,
    uint8_t type,
    uint32_t seq
) {
    uint8_t resp[HANDSHAKE_PACKET_SIZE];
    resp[0] = static_cast<uint8_t>(PROTOCOL_MAGIC & 0xFF);
    resp[1] = static_cast<uint8_t>((PROTOCOL_MAGIC >> 8) & 0xFF);
    resp[2] = PROTOCOL_VERSION;
    resp[3] = type;
    resp[4] = static_cast<uint8_t>(seq & 0xFF);
    resp[5] = static_cast<uint8_t>((seq >> 8) & 0xFF);
    resp[6] = static_cast<uint8_t>((seq >> 16) & 0xFF);
    resp[7] = static_cast<uint8_t>((seq >> 24) & 0xFF);
    resp[8] = 0; resp[9] = 0; resp[10] = 0; resp[11] = 0;

    sendto(sock, (const char*)resp, sizeof(resp), 0, (const sockaddr*)&targetAddr, sizeof(targetAddr));
}

std::string UdpReceiver::getClientIp() const {
    std::lock_guard<std::mutex> lock(m_clientMutex);
    return m_clientIp;
}

} // namespace GamepadReceiver
