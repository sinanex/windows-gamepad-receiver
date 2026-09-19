#include "ControllerProcessor.h"

namespace GamepadReceiver {

ControllerProcessor::ControllerProcessor(Statistics& statistics) noexcept
    : m_stats(statistics),
      m_lastValidTime(std::chrono::steady_clock::now()) {}

bool ControllerProcessor::processPacket(const ControllerState& incoming) noexcept {
    const auto now = std::chrono::steady_clock::now();

    if (!m_hasReceivedFirstPacket.load(std::memory_order_relaxed)) {
        m_hasReceivedFirstPacket.store(true, std::memory_order_relaxed);
        m_lastSequence.store(incoming.sequence, std::memory_order_relaxed);
        m_isFailsafeActive.store(false, std::memory_order_relaxed);

        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            m_latestState = incoming;
            m_lastValidTime = now;
        }

        m_stats.onPacketReceived(incoming.sequence, incoming.timestampNs);
        return true;
    }

    const uint32_t lastSeq = m_lastSequence.load(std::memory_order_relaxed);
    const int32_t diff = static_cast<int32_t>(incoming.sequence - lastSeq);

    // Sequence wrap-around and monotonic check:
    // If diff > 0: packet is newer
    // If diff <= 0: packet is duplicate or older out-of-order -> discard!
    if (diff <= 0) {
        m_stats.onPacketOutOfOrder();
        return false;
    }

    // Packet loss detection: if diff > 1, packets were lost in transit
    if (diff > 1) {
        m_stats.onPacketDropped(static_cast<uint64_t>(diff - 1));
    }

    m_lastSequence.store(incoming.sequence, std::memory_order_relaxed);
    m_isFailsafeActive.store(false, std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        m_latestState = incoming;
        m_lastValidTime = now;
    }

    m_stats.onPacketReceived(incoming.sequence, incoming.timestampNs);
    return true;
}

bool ControllerProcessor::checkFailsafe(int timeoutMs) noexcept {
    const auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(m_stateMutex);

    if (!m_hasReceivedFirstPacket.load(std::memory_order_relaxed)) {
        return false;
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastValidTime).count();
    if (elapsed > timeoutMs) {
        if (!m_isFailsafeActive.load(std::memory_order_relaxed)) {
            m_latestState.resetToNeutral();
            m_isFailsafeActive.store(true, std::memory_order_relaxed);
            return true; // Newly triggered failsafe reset
        }
    }
    return false;
}

bool ControllerProcessor::isTimedOut(int timeoutMs) const noexcept {
    const auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(m_stateMutex);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastValidTime).count();
    return elapsed > timeoutMs;
}

ControllerState ControllerProcessor::getLatestState() const noexcept {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_latestState;
}

void ControllerProcessor::resetToNeutral() noexcept {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_latestState.resetToNeutral();
    m_isFailsafeActive.store(true, std::memory_order_relaxed);
}

} // namespace GamepadReceiver
