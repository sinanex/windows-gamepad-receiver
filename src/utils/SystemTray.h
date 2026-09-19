#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <string>

namespace GamepadReceiver {

class SystemTray {
public:
    SystemTray(HWND hWnd, UINT uCallbackMessage, UINT uId = 1);
    ~SystemTray();

    bool add(const std::wstring& tooltip);
    bool updateTooltip(const std::wstring& tooltip);
    bool remove();
    void showContextMenu();

    static constexpr UINT WM_TRAY_CALLBACK = WM_USER + 101;
    static constexpr UINT ID_TRAY_OPEN    = 2001;
    static constexpr UINT ID_TRAY_START   = 2002;
    static constexpr UINT ID_TRAY_STOP    = 2003;
    static constexpr UINT ID_TRAY_EXIT    = 2004;

private:
    HWND m_hWnd;
    NOTIFYICONDATAW m_nid{};
    bool m_installed{false};
};

} // namespace GamepadReceiver
