#pragma once

#include <string>
#include "../controller/ControllerState.h"

namespace GamepadReceiver {

enum class GamepadBackendType {
    ViGEmXbox360,
    MockEmulated
};

enum class GamepadStatus {
    Uninitialized,
    DriverMissing,
    Connected,
    Disconnected,
    Error
};

class VirtualGamepad {
public:
    virtual ~VirtualGamepad() = default;

    virtual bool initialize() = 0;
    virtual bool connect() = 0;
    virtual bool update(const ControllerState& state) = 0;
    virtual void reset() = 0;
    virtual void disconnect() = 0;

    virtual GamepadStatus getStatus() const noexcept = 0;
    virtual GamepadBackendType getType() const noexcept = 0;
    virtual std::string getStatusMessage() const = 0;
};

} // namespace GamepadReceiver
