#pragma once

#include "VirtualGamepad.h"
#include <windows.h>

namespace GamepadReceiver {

class ViGEmGamepad : public VirtualGamepad {
public:
    ViGEmGamepad();
    ~ViGEmGamepad() override;

    bool initialize() override;
    bool connect() override;
    bool update(const ControllerState& state) override;
    void reset() override;
    void disconnect() override;

    GamepadStatus getStatus() const noexcept override { return m_status; }
    GamepadBackendType getType() const noexcept override { return GamepadBackendType::ViGEmXbox360; }
    std::string getStatusMessage() const override { return m_statusMessage; }

private:
    GamepadStatus m_status{GamepadStatus::Uninitialized};
    std::string   m_statusMessage{"Uninitialized"};

    HMODULE m_hViGEmDll{nullptr};
    void*   m_client{nullptr};
    void*   m_target{nullptr};

    // Function pointers for dynamic ViGEmClient binding
    typedef void* (*pfn_vigem_alloc)();
    typedef void  (*pfn_vigem_free)(void*);
    typedef int   (*pfn_vigem_connect)(void*);
    typedef void  (*pfn_vigem_disconnect)(void*);
    typedef void* (*pfn_vigem_target_x360_alloc)();
    typedef void  (*pfn_vigem_target_free)(void*);
    typedef int   (*pfn_vigem_target_add)(void*, void*);
    typedef int   (*pfn_vigem_target_remove)(void*, void*);
    typedef int   (*pfn_vigem_target_x360_update)(void*, void*, void*);

    pfn_vigem_alloc              m_pfnAlloc{nullptr};
    pfn_vigem_free               m_pfnFree{nullptr};
    pfn_vigem_connect            m_pfnConnect{nullptr};
    pfn_vigem_disconnect         m_pfnDisconnect{nullptr};
    pfn_vigem_target_x360_alloc  m_pfnX360Alloc{nullptr};
    pfn_vigem_target_free        m_pfnTargetFree{nullptr};
    pfn_vigem_target_add         m_pfnTargetAdd{nullptr};
    pfn_vigem_target_remove      m_pfnTargetRemove{nullptr};
    pfn_vigem_target_x360_update m_pfnX360Update{nullptr};
};

} // namespace GamepadReceiver
