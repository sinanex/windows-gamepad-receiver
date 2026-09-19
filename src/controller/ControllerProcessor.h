#pragma once

#include <cstdint>
#include <chrono>
#include <atomic>
#include <mutex>
#include "ControllerState.h"
#include "../diagnostics/Statistics.h"

namespace GamepadReceiver {

class ControllerProcessor {
public:
    explicit ControllerProcessor(Statistics& statistics) noexcept;

    /**
     * Process newly decoded state.
     * Evaluates sequence ordering, rejects stale/out-of-order packets,
     * updates latest state, and records diagnostic stats.
     * 
     * @return true if accepted as newer valid state, false if dropped
     */
    bool processPacket(const ControllerState& incoming) noexcept;

    /**
     * Checks if watchdog timeout (e.g. 300ms) has elapsed since last valid packet.
     * If timed out, resets the controller to neutral state.
     * 
     * @return true if timeout occurred and reset triggered
     */
    bool checkFailsafe(int timeoutMs = 300) noexcept;

    ControllerState getLatestState() const noexcept;
    void resetToNeutral() noexcept;

    uint32_t getLastSequence() const noexcept { return m_lastSequence.load(std::memory_order_relaxed); }
    bool isTimedOut(int timeoutMs = 300) const noexcept;

private:
    Statistics& m_stats;

    mutable std::mutex m_stateMutex;
    ControllerState m_latestState;

    std::atomic<uint32_t> m_lastSequence{0};
    std::atomic<bool>     m_hasReceivedFirstPacket{false};
    std::atomic<bool>     m_isFailsafeActive{false};

    std::chrono::steady_clock::time_point m_lastValidTime;
};

} // namespace GamepadReceiver
