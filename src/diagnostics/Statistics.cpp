#include "Statistics.h"

namespace GamepadReceiver {

Statistics::Statistics() noexcept 
    : m_startTime(std::chrono::steady_clock::now()),
      m_lastPacketTime(std::chrono::steady_clock::now()) {}

void Statistics::onPacketReceived(uint32_t sequence, int64_t timestampNs) noexcept {
    m_packetsReceived.fetch_add(1, std::memory_order_relaxed);
    m_secCounter.fetch_add(1, std::memory_order_relaxed);
    m_lastSequence.store(sequence, std::memory_order_relaxed);

    const auto now = std::chrono::steady_clock::now();
    m_lastPacketTime = now;
}

void Statistics::onPacketDropped(uint64_t count) noexcept {
    m_packetsLost.fetch_add(count, std::memory_order_relaxed);
}

void Statistics::onPacketOutOfOrder() noexcept {
    m_packetsOutOfOrder.fetch_add(1, std::memory_order_relaxed);
}

void Statistics::onControllerUpdated() noexcept {
    m_controllerUpdates.fetch_add(1, std::memory_order_relaxed);
}

void Statistics::onGamepadError() noexcept {
    m_gamepadErrors.fetch_add(1, std::memory_order_relaxed);
}

void Statistics::onRttMeasured(int64_t rttMs) noexcept {
    m_estimatedRttMs.store(rttMs, std::memory_order_relaxed);
}

void Statistics::tickOneSecond() noexcept {
    m_packetsPerSec.store(m_secCounter.exchange(0, std::memory_order_relaxed), std::memory_order_relaxed);
}

StatisticsSnapshot Statistics::getSnapshot() const noexcept {
    StatisticsSnapshot s;
    s.packetsReceived    = m_packetsReceived.load(std::memory_order_relaxed);
    s.packetsLost        = m_packetsLost.load(std::memory_order_relaxed);
    s.packetsOutOfOrder  = m_packetsOutOfOrder.load(std::memory_order_relaxed);
    s.packetsPerSec      = m_packetsPerSec.load(std::memory_order_relaxed);
    s.lastSequence       = m_lastSequence.load(std::memory_order_relaxed);
    s.estimatedRttMs     = m_estimatedRttMs.load(std::memory_order_relaxed);
    s.controllerUpdates  = m_controllerUpdates.load(std::memory_order_relaxed);
    s.gamepadErrors      = m_gamepadErrors.load(std::memory_order_relaxed);

    const auto now = std::chrono::steady_clock::now();
    s.uptimeSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - m_startTime).count();
    s.packetAgeMs   = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastPacketTime).count();
    return s;
}

void Statistics::reset() noexcept {
    m_packetsReceived.store(0);
    m_packetsLost.store(0);
    m_packetsOutOfOrder.store(0);
    m_packetsPerSec.store(0);
    m_lastSequence.store(0);
    m_secCounter.store(0);
    m_controllerUpdates.store(0);
    m_gamepadErrors.store(0);
    m_startTime = std::chrono::steady_clock::now();
    m_lastPacketTime = m_startTime;
}

} // namespace GamepadReceiver
