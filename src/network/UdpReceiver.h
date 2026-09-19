#pragma once

#include <cstdint>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include "../controller/ControllerProcessor.h"
#include "../gamepad/VirtualGamepad.h"
#include "../diagnostics/Statistics.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace GamepadReceiver {

enum class ReceiverState {
    Stopped,
    Listening,
    Connected,
    Error
};

class UdpReceiver {
public:
    UdpReceiver(
        ControllerProcessor& processor,
        VirtualGamepad* gamepad,
        Statistics& statistics
    );
    ~UdpReceiver();

    bool start(uint16_t port = 5000);
    void stop();

    void setGamepad(VirtualGamepad* gamepad) noexcept { m_gamepad = gamepad; }

    ReceiverState getState() const noexcept { return m_state.load(); }
    uint16_t getPort() const noexcept { return m_port; }
    std::string getClientIp() const;
    uint16_t getClientPort() const noexcept { return m_clientPort.load(); }

    void setStatusCallback(std::function<void(ReceiverState)> callback) {
        m_statusCallback = callback;
    }

private:
    void receiverLoop();
    void sendHandshakeResponse(SOCKET sock, const sockaddr_in& targetAddr, uint8_t type, uint32_t seq);

    ControllerProcessor& m_processor;
    VirtualGamepad*      m_gamepad{nullptr};
    Statistics&          m_stats;

    uint16_t             m_port{5000};
    std::atomic<ReceiverState> m_state{ReceiverState::Stopped};
    std::atomic<bool>    m_running{false};
    std::thread          m_receiverThread;

    SOCKET               m_socket{INVALID_SOCKET};

    mutable std::mutex   m_clientMutex;
    std::string          m_clientIp{"0.0.0.0"};
    std::atomic<uint16_t> m_clientPort{0};

    std::function<void(ReceiverState)> m_statusCallback;
};

} // namespace GamepadReceiver
