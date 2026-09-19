#pragma once

#include "VirtualGamepad.h"
#include "GamepadMapper.h"
#include <mutex>

namespace GamepadReceiver {

class MockGamepad : public VirtualGamepad {
public:
    MockGamepad() = default;

    bool initialize() override {
        m_status = GamepadStatus::Connected;
        return true;
    }

    bool connect() override {
        m_status = GamepadStatus::Connected;
        return true;
    }

    bool update(const ControllerState& state) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_latestState = state;
        m_latestXInput = GamepadMapper::mapToXInput(state);
        return true;
    }

    void reset() override {
        ControllerState neutral{};
        update(neutral);
    }

    void disconnect() override {
        m_status = GamepadStatus::Disconnected;
    }

    GamepadStatus getStatus() const noexcept override { return m_status; }
    GamepadBackendType getType() const noexcept override { return GamepadBackendType::MockEmulated; }
    std::string getStatusMessage() const override {
        return "Mock Emulated Controller (Testing Mode)";
    }

    XInputReport getLatestReport() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_latestXInput;
    }

private:
    mutable std::mutex m_mutex;
    GamepadStatus m_status{GamepadStatus::Connected};
    ControllerState m_latestState{};
    XInputReport m_latestXInput{};
};

} // namespace GamepadReceiver
