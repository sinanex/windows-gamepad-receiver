#include "SystemTray.h"

namespace GamepadReceiver {

SystemTray::SystemTray(HWND hWnd, UINT uCallbackMessage, UINT uId)
    : m_hWnd(hWnd) {
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = hWnd;
    m_nid.uID = uId;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = uCallbackMessage;
    m_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
}

SystemTray::~SystemTray() {
    remove();
}

bool SystemTray::add(const std::wstring& tooltip) {
    wcsncpy_s(m_nid.szTip, tooltip.c_str(), _TRUNCATE);
    m_installed = Shell_NotifyIconW(NIM_ADD, &m_nid) != FALSE;
    return m_installed;
}

bool SystemTray::updateTooltip(const std::wstring& tooltip) {
    if (!m_installed) return false;
    wcsncpy_s(m_nid.szTip, tooltip.c_str(), _TRUNCATE);
    return Shell_NotifyIconW(NIM_MODIFY, &m_nid) != FALSE;
}

bool SystemTray::remove() {
    if (m_installed) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_installed = false;
        return true;
    }
    return false;
}

void SystemTray::showContextMenu() {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, ID_TRAY_OPEN, L"Open Controller Window");
    InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
    InsertMenuW(hMenu, 2, MF_BYPOSITION | MF_STRING, ID_TRAY_START, L"Start Receiver");
    InsertMenuW(hMenu, 3, MF_BYPOSITION | MF_STRING, ID_TRAY_STOP, L"Stop Receiver");
    InsertMenuW(hMenu, 4, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
    InsertMenuW(hMenu, 5, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT, L"Exit");

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(m_hWnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, m_hWnd, nullptr);
    DestroyMenu(hMenu);
}

} // namespace GamepadReceiver
