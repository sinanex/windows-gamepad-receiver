#include "ConnectionManager.h"
#include "../gamepad/ViGEmGamepad.h"
#include "../gamepad/MockGamepad.h"
#include "../utils/Logger.h"

namespace GamepadReceiver {

ConnectionManager::ConnectionManager()
    : m_stats(),
      m_processor(m_stats),
      m_gamepad(nullptr),
      m_receiver(m_processor, nullptr, m_stats)
{
    // Try to load ViGEm virtual controller backend
    auto vigem = std::make_unique<ViGEmGamepad>();
    if (vigem->initialize()) {
        LOG_INFO("ViGEmBus backend initialized successfully");
        m_gamepad = std::move(vigem);
    } else {
        LOG_WARN("ViGEmBus not found (" + vigem->getStatusMessage() + "). Falling back to MockGamepad.");
        m_gamepad = std::make_unique<MockGamepad>();
        m_gamepad->initialize();
    }
    m_receiver.setGamepad(m_gamepad.get());
}

ConnectionManager::~ConnectionManager() {
    stop();
}

bool ConnectionManager::start(uint16_t port) {
    if (m_gamepad) {
        m_gamepad->connect();
    }
    return m_receiver.start(port);
}

void ConnectionManager::stop() {
    m_receiver.stop();
    if (m_gamepad) {
        m_gamepad->reset();
        m_gamepad->disconnect();
    }
}

bool ConnectionManager::isConnected() const noexcept {
    return m_receiver.getState() == ReceiverState::Connected;
}

std::string ConnectionManager::getStatusString() const {
    switch (m_receiver.getState()) {
        case ReceiverState::Stopped:   return "STOPPED";
        case ReceiverState::Listening: return "LISTENING (Waiting for Android)";
        case ReceiverState::Connected: return "CONNECTED (" + m_receiver.getClientIp() + ")";
        case ReceiverState::Error:     return "ERROR";
    }
    return "UNKNOWN";
}

std::string ConnectionManager::getGamepadBackendString() const {
    if (!m_gamepad) return "None";
    return m_gamepad->getStatusMessage();
}

} // namespace GamepadReceiver
