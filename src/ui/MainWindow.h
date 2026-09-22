#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <memory>
#include <vector>
#include <string>
#include "../connection/ConnectionManager.h"
#include "../utils/SystemTray.h"

namespace GamepadReceiver {

class MainWindow {
public:
    MainWindow(HINSTANCE hInstance, ConnectionManager& connectionManager);
    ~MainWindow();

    bool create(int nCmdShow);
    int runMessageLoop();

private:
    static LRESULT CALLBACK WndProcSetup(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK WndProcThunk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void onPaint(HWND hWnd);
    void drawStick(HDC hdc, int centerX, int centerY, int radius, int16_t x, int16_t y, const wchar_t* label);
    void drawTrigger(HDC hdc, int x, int y, int width, int height, uint16_t val, const wchar_t* label);
    void drawButton(HDC hdc, int x, int y, int radius, bool pressed, COLORREF activeColor, const wchar_t* label);
    void drawDpad(HDC hdc, int centerX, int centerY, uint8_t dpadMask);

    // QR code support
    void drawQrCode(HDC hdc, int x, int y, int moduleSize) const;
    void regenerateQr();
    static std::string getLocalIpAddress();

    HINSTANCE m_hInstance;
    HWND      m_hWnd{nullptr};
    HWND      m_btnToggle{nullptr};
    HWND      m_editPort{nullptr};

    ConnectionManager& m_connectionManager;
    std::unique_ptr<SystemTray> m_tray;

    // QR code cache  (regenerated when IP/port changes)
    std::vector<std::vector<bool>> m_qrMatrix;
    std::string m_qrLastEncoded; // what was encoded, to detect changes

    static constexpr UINT ID_BTN_TOGGLE  = 1001;
    static constexpr UINT ID_EDIT_PORT   = 1002;
    static constexpr UINT TIMER_UI_REFRESH = 1;
};

} // namespace GamepadReceiver
