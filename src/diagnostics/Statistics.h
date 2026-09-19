#pragma once

#include <cstdint>
#include <atomic>
#include <chrono>

namespace GamepadReceiver {

struct StatisticsSnapshot {
    uint64_t packetsReceived{0};
    uint64_t packetsLost{0};
    uint64_t packetsOutOfOrder{0};
    uint32_t packetsPerSec{0};
    uint32_t lastSequence{0};
    int64_t  packetAgeMs{0};
    int64_t  estimatedRttMs{-1};
    uint64_t controllerUpdates{0};
    uint64_t gamepadErrors{0};
    uint64_t uptimeSeconds{0};
};

class Statistics {
public:
    Statistics() noexcept;

    void onPacketReceived(uint32_t sequence, int64_t timestampNs) noexcept;
    void onPacketDropped(uint64_t count = 1) noexcept;
    void onPacketOutOfOrder() noexcept;
    void onControllerUpdated() noexcept;
    void onGamepadError() noexcept;
    void onRttMeasured(int64_t rttMs) noexcept;

    void tickOneSecond() noexcept;

    StatisticsSnapshot getSnapshot() const noexcept;
    void reset() noexcept;

private:
    std::atomic<uint64_t> m_packetsReceived{0};
    std::atomic<uint64_t> m_packetsLost{0};
    std::atomic<uint64_t> m_packetsOutOfOrder{0};
    std::atomic<uint32_t> m_packetsPerSec{0};
    std::atomic<uint32_t> m_lastSequence{0};
    std::atomic<int64_t>  m_packetAgeMs{0};
    std::atomic<int64_t>  m_estimatedRttMs{-1};
    std::atomic<uint64_t> m_controllerUpdates{0};
    std::atomic<uint64_t> m_gamepadErrors{0};

    std::atomic<uint32_t> m_secCounter{0};
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_lastPacketTime;
};

} // namespace GamepadReceiver
