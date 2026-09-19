#include "ViGEmGamepad.h"
#include "GamepadMapper.h"

namespace GamepadReceiver {

// Binary representation of XUSB_REPORT expected by ViGEmBus
#pragma pack(push, 1)
struct ViGEmX360Report {
    uint16_t wButtons;
    uint8_t  bLeftTrigger;
    uint8_t  bRightTrigger;
    int16_t  sThumbLX;
    int16_t  sThumbLY;
    int16_t  sThumbRX;
    int16_t  sThumbRY;
};
#pragma pack(pop)

ViGEmGamepad::ViGEmGamepad() = default;

ViGEmGamepad::~ViGEmGamepad() {
    disconnect();
}

bool ViGEmGamepad::initialize() {
    // Attempt to load ViGEmClient.dll dynamically
    m_hViGEmDll = LoadLibraryW(L"ViGEmClient.dll");
    if (!m_hViGEmDll) {
        m_status = GamepadStatus::DriverMissing;
        m_statusMessage = "ViGEmClient.dll not found. Please install ViGEmBus driver (Nefarius).";
        return false;
    }

    m_pfnAlloc        = (pfn_vigem_alloc)GetProcAddress(m_hViGEmDll, "vigem_alloc");
    m_pfnFree         = (pfn_vigem_free)GetProcAddress(m_hViGEmDll, "vigem_free");
    m_pfnConnect      = (pfn_vigem_connect)GetProcAddress(m_hViGEmDll, "vigem_connect");
    m_pfnDisconnect   = (pfn_vigem_disconnect)GetProcAddress(m_hViGEmDll, "vigem_disconnect");
    m_pfnX360Alloc    = (pfn_vigem_target_x360_alloc)GetProcAddress(m_hViGEmDll, "vigem_target_x360_alloc");
    m_pfnTargetFree   = (pfn_vigem_target_free)GetProcAddress(m_hViGEmDll, "vigem_target_free");
    m_pfnTargetAdd    = (pfn_vigem_target_add)GetProcAddress(m_hViGEmDll, "vigem_target_add");
    m_pfnTargetRemove = (pfn_vigem_target_remove)GetProcAddress(m_hViGEmDll, "vigem_target_remove");
    m_pfnX360Update   = (pfn_vigem_target_x360_update)GetProcAddress(m_hViGEmDll, "vigem_target_x360_update");

    if (!m_pfnAlloc || !m_pfnConnect || !m_pfnX360Alloc || !m_pfnTargetAdd || !m_pfnX360Update) {
        m_status = GamepadStatus::Error;
        m_statusMessage = "ViGEmClient.dll missing required export functions.";
        FreeLibrary(m_hViGEmDll);
        m_hViGEmDll = nullptr;
        return false;
    }

    return connect();
}

bool ViGEmGamepad::connect() {
    if (!m_hViGEmDll || !m_pfnAlloc) return false;

    m_client = m_pfnAlloc();
    if (!m_client) {
        m_status = GamepadStatus::Error;
        m_statusMessage = "Failed to allocate ViGEm client context.";
        return false;
    }

    int connectRes = m_pfnConnect(m_client);
    if (connectRes != 0) { // VIGEM_ERROR_NONE = 0x20000000 or 0
        m_status = GamepadStatus::DriverMissing;
        m_statusMessage = "Cannot connect to ViGEmBus driver. Ensure ViGEmBus is running.";
        m_pfnFree(m_client);
        m_client = nullptr;
        return false;
    }

    m_target = m_pfnX360Alloc();
    if (!m_target) {
        m_status = GamepadStatus::Error;
        m_statusMessage = "Failed to allocate Xbox 360 virtual gamepad target.";
        return false;
    }

    int addRes = m_pfnTargetAdd(m_client, m_target);
    if (addRes != 0) {
        m_status = GamepadStatus::Error;
        m_statusMessage = "Failed to attach virtual controller to ViGEmBus.";
        return false;
    }

    m_status = GamepadStatus::Connected;
    m_statusMessage = "Virtual Xbox 360 Gamepad Connected via ViGEmBus";
    return true;
}

bool ViGEmGamepad::update(const ControllerState& state) {
    if (m_status != GamepadStatus::Connected || !m_client || !m_target || !m_pfnX360Update) {
        return false;
    }

    const XInputReport xInput = GamepadMapper::mapToXInput(state);

    ViGEmX360Report report{};
    report.wButtons      = xInput.wButtons;
    report.bLeftTrigger  = xInput.bLeftTrigger;
    report.bRightTrigger = xInput.bRightTrigger;
    report.sThumbLX      = xInput.sThumbLX;
    report.sThumbLY      = xInput.sThumbLY;
    report.sThumbRX      = xInput.sThumbRX;
    report.sThumbRY      = xInput.sThumbRY;

    int res = m_pfnX360Update(m_client, m_target, &report);
    return (res == 0);
}

void ViGEmGamepad::reset() {
    ControllerState neutral{};
    update(neutral);
}

void ViGEmGamepad::disconnect() {
    if (m_client && m_target && m_pfnTargetRemove) {
        m_pfnTargetRemove(m_client, m_target);
    }
    if (m_target && m_pfnTargetFree) {
        m_pfnTargetFree(m_target);
        m_target = nullptr;
    }
    if (m_client && m_pfnDisconnect) {
        m_pfnDisconnect(m_client);
    }
    if (m_client && m_pfnFree) {
        m_pfnFree(m_client);
        m_client = nullptr;
    }
    if (m_hViGEmDll) {
        FreeLibrary(m_hViGEmDll);
        m_hViGEmDll = nullptr;
    }
    m_status = GamepadStatus::Disconnected;
    m_statusMessage = "Disconnected";
}

} // namespace GamepadReceiver
