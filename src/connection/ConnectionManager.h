#pragma once

#include <memory>
#include <string>
#include "../network/UdpReceiver.h"
#include "../controller/ControllerProcessor.h"
#include "../gamepad/VirtualGamepad.h"
#include "../diagnostics/Statistics.h"

namespace GamepadReceiver {

class ConnectionManager {
public:
    ConnectionManager();
    ~ConnectionManager();

    bool start(uint16_t port = 5000);
    void stop();

    bool isConnected() const noexcept;
    std::string getStatusString() const;
    std::string getGamepadBackendString() const;

    Statistics& getStatistics() noexcept { return m_stats; }
    ControllerProcessor& getProcessor() noexcept { return m_processor; }
    UdpReceiver& getReceiver() noexcept { return m_receiver; }
    VirtualGamepad& getGamepad() noexcept { return *m_gamepad; }

private:
    Statistics          m_stats;
    ControllerProcessor m_processor;
    std::unique_ptr<VirtualGamepad> m_gamepad;
    UdpReceiver         m_receiver;
};

} // namespace GamepadReceiver
