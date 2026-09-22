#include "MainWindow.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

// QR code (single-header, zero-dependency encoder)
#include "../utils/QrCode.h"

// IP detection
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>

namespace GamepadReceiver {

MainWindow::MainWindow(HINSTANCE hInstance, ConnectionManager& connectionManager)
    : m_hInstance(hInstance),
      m_connectionManager(connectionManager) {}

MainWindow::~MainWindow() {
    if (m_hWnd) {
        KillTimer(m_hWnd, TIMER_UI_REFRESH);
    }
}

bool MainWindow::create(int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"VirtualGamepadReceiverWindowClass";

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProcSetup;
    wc.hInstance     = m_hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);

    RegisterClassExW(&wc);

    m_hWnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Virtual Gamepad Receiver (120Hz UDP)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 560,
        nullptr, nullptr, m_hInstance, this
    );

    if (!m_hWnd) return false;

    // Controls: Port input & Toggle Start/Stop button
    CreateWindowExW(0, L"STATIC", L"UDP Port:", WS_CHILD | WS_VISIBLE, 24, 18, 70, 20, m_hWnd, nullptr, m_hInstance, nullptr);
    m_editPort = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"5000", WS_CHILD | WS_VISIBLE | ES_NUMBER, 98, 15, 65, 24, m_hWnd, (HMENU)ID_EDIT_PORT, m_hInstance, nullptr);
    m_btnToggle = CreateWindowExW(0, L"BUTTON", L"Stop Receiver", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 175, 14, 110, 26, m_hWnd, (HMENU)ID_BTN_TOGGLE, m_hInstance, nullptr);

    // Initialize System Tray
    m_tray = std::make_unique<SystemTray>(m_hWnd, SystemTray::WM_TRAY_CALLBACK);
    m_tray->add(L"Virtual Gamepad Receiver (Active)");

    // 15Hz decoupled UI timer (every ~66ms)
    SetTimer(m_hWnd, TIMER_UI_REFRESH, 66, nullptr);

    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);
    return true;
}

int MainWindow::runMessageLoop() {
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK MainWindow::WndProcSetup(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* window = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        SetWindowLongPtrW(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcThunk));
        return window->handleMessage(hWnd, uMsg, wParam, lParam);
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK MainWindow::WndProcThunk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto* window = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    if (window) {
        return window->handleMessage(hWnd, uMsg, wParam, lParam);
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

LRESULT MainWindow::handleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_TIMER:
            if (wParam == TIMER_UI_REFRESH) {
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            return 0;

        case WM_PAINT:
            onPaint(hWnd);
            return 0;

        case WM_COMMAND: {
            if (LOWORD(wParam) == ID_BTN_TOGGLE) {
                if (m_connectionManager.getReceiver().getState() == ReceiverState::Stopped) {
                    wchar_t portBuf[16]{};
                    GetWindowTextW(m_editPort, portBuf, 16);
                    uint16_t port = (uint16_t)_wtoi(portBuf);
                    if (port == 0) port = 5000;
                    m_connectionManager.start(port);
                    SetWindowTextW(m_btnToggle, L"Stop Receiver");
                } else {
                    m_connectionManager.stop();
                    SetWindowTextW(m_btnToggle, L"Start Receiver");
                }
            }
            return 0;
        }

        case SystemTray::WM_TRAY_CALLBACK: {
            if (lParam == WM_RBUTTONUP) {
                m_tray->showContextMenu();
            } else if (lParam == WM_LBUTTONDBLCLK) {
                ShowWindow(hWnd, SW_RESTORE);
                SetForegroundWindow(hWnd);
            }
            return 0;
        }

        case WM_COMMAND + 1000: // Tray menu commands
        case WM_SYSCOMMAND:
            if (wParam == SC_MINIMIZE) {
                // Minimize to tray
                ShowWindow(hWnd, SW_HIDE);
                return 0;
            }
            break;

        case WM_DESTROY:
            if (m_tray) m_tray->remove();
            PostQuitMessage(0);
            return 0;
    }

    if (uMsg == WM_COMMAND) {
        switch (LOWORD(wParam)) {
            case SystemTray::ID_TRAY_OPEN:
                ShowWindow(hWnd, SW_RESTORE);
                SetForegroundWindow(hWnd);
                return 0;
            case SystemTray::ID_TRAY_START:
                m_connectionManager.start(5000);
                SetWindowTextW(m_btnToggle, L"Stop Receiver");
                return 0;
            case SystemTray::ID_TRAY_STOP:
                m_connectionManager.stop();
                SetWindowTextW(m_btnToggle, L"Start Receiver");
                return 0;
            case SystemTray::ID_TRAY_EXIT:
                DestroyWindow(hWnd);
                return 0;
        }
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

void MainWindow::onPaint(HWND hWnd) {
    PAINTSTRUCT ps;
    HDC hdcWindow = BeginPaint(hWnd, &ps);

    RECT rc;
    GetClientRect(hWnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    // Double buffering to eliminate flicker
    HDC hdc = CreateCompatibleDC(hdcWindow);
    HBITMAP hbm = CreateCompatibleBitmap(hdcWindow, width, height);
    HBITMAP oldBmp = (HBITMAP)SelectObject(hdc, hbm);

    // Background: Dark Slate
    HBRUSH bgBrush = CreateSolidBrush(RGB(15, 23, 42));
    FillRect(hdc, &rc, bgBrush);
    DeleteObject(bgBrush);

    SetBkMode(hdc, TRANSPARENT);

    // Status Pill
    const auto status = m_connectionManager.getReceiver().getState();
    COLORREF statusColor = (status == ReceiverState::Connected) ? RGB(16, 185, 129) :
                           (status == ReceiverState::Listening) ? RGB(245, 158, 11) : RGB(100, 116, 139);

    HBRUSH pillBrush = CreateSolidBrush(statusColor);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, pillBrush);
    Ellipse(hdc, 310, 19, 324, 33);
    SelectObject(hdc, oldBrush);
    DeleteObject(pillBrush);

    SetTextColor(hdc, RGB(241, 245, 249));
    std::string rawStatus = m_connectionManager.getStatusString();
    std::wstring statusStr = L"STATUS: " + std::wstring(rawStatus.begin(), rawStatus.end());
    TextOutW(hdc, 332, 18, statusStr.c_str(), (int)statusStr.length());

    // Backend info
    SetTextColor(hdc, RGB(148, 163, 184));
    std::string rawBackend = m_connectionManager.getGamepadBackendString();
    std::wstring backendStr = L"Backend: " + std::wstring(rawBackend.begin(), rawBackend.end());
    TextOutW(hdc, 24, 52, backendStr.c_str(), (int)backendStr.length());

    // Divider
    HPEN penDivider = CreatePen(PS_SOLID, 1, RGB(30, 41, 59));
    HPEN oldPen = (HPEN)SelectObject(hdc, penDivider);
    MoveToEx(hdc, 20, 78, nullptr);
    LineTo(hdc, width - 20, 78);
    SelectObject(hdc, oldPen);
    DeleteObject(penDivider);

    // Telemetry Statistics Box
    const auto stats = m_connectionManager.getStatistics().getSnapshot();
    std::wstringstream ss;
    ss << L"Packets: " << stats.packetsReceived << L"  |  Actual Rate: " << stats.packetsPerSec
       << L" Hz  |  Seq: #" << stats.lastSequence << L"  |  Dropped: " << stats.packetsLost
       << L"  |  Age: " << stats.packetAgeMs << L" ms";
    std::wstring statsStr = ss.str();

    SetTextColor(hdc, RGB(56, 189, 248));
    TextOutW(hdc, 24, 88, statsStr.c_str(), (int)statsStr.length());

    // Live Gamepad Visualizer Zone
    const auto state = m_connectionManager.getProcessor().getLatestState();

    // 1. Left Analog Stick
    drawStick(hdc, 120, 230, 60, state.leftX, state.leftY, L"LEFT STICK");

    // 2. D-Pad
    drawDpad(hdc, 120, 390, state.dpad);

    // 3. Triggers & Bumpers
    drawTrigger(hdc, 240, 140, 110, 24, state.leftTrigger, L"LT");
    drawButton(hdc, 380, 140, 18, state.isButtonPressed(BTN_L1), RGB(99, 102, 241), L"LB");

    drawButton(hdc, 460, 140, 18, state.isButtonPressed(BTN_R1), RGB(99, 102, 241), L"RB");
    drawTrigger(hdc, 500, 140, 110, 24, state.rightTrigger, L"RT");

    // 4. System Buttons (Select / Start)
    drawButton(hdc, 320, 220, 16, state.isButtonPressed(BTN_SELECT), RGB(148, 163, 184), L"BACK");
    drawButton(hdc, 420, 220, 16, state.isButtonPressed(BTN_START), RGB(148, 163, 184), L"START");

    // 5. Action Buttons Cluster (Y, X, B, A)
    drawButton(hdc, 580, 240, 20, state.isButtonPressed(BTN_Y), RGB(251, 191, 36), L"Y");
    drawButton(hdc, 530, 280, 20, state.isButtonPressed(BTN_X), RGB(56, 189, 248), L"X");
    drawButton(hdc, 630, 280, 20, state.isButtonPressed(BTN_B), RGB(248, 113, 113), L"B");
    drawButton(hdc, 580, 320, 20, state.isButtonPressed(BTN_A), RGB(52, 211, 153), L"A");

    // 6. Right Analog Stick
    drawStick(hdc, 430, 370, 60, state.rightX, state.rightY, L"RIGHT STICK");

    // ── QR Code Panel ─────────────────────────────────────────────────────
    // Panel background
    int qrPanelX = 750;
    RECT qrPanel{qrPanelX, 78, width - 10, height - 10};
    HBRUSH qrPanelBrush = CreateSolidBrush(RGB(21, 31, 52));
    FillRect(hdc, &qrPanel, qrPanelBrush);
    DeleteObject(qrPanelBrush);

    // Panel border
    HPEN qrBorder = CreatePen(PS_SOLID, 1, RGB(51, 65, 85));
    HPEN qrOldPen = (HPEN)SelectObject(hdc, qrBorder);
    HBRUSH qrNullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH qrOldBrush = (HBRUSH)SelectObject(hdc, qrNullBrush);
    Rectangle(hdc, qrPanelX, 78, width - 10, height - 10);
    SelectObject(hdc, qrOldPen);
    SelectObject(hdc, qrOldBrush);
    DeleteObject(qrBorder);

    // Title label
    SetTextColor(hdc, RGB(148, 163, 184));
    TextOutW(hdc, qrPanelX + 14, 90, L"Scan to Connect", 15);

    // Regenerate QR if IP/port changed or not yet generated
    auto recvState = m_connectionManager.getReceiver().getState();
    if (recvState != ReceiverState::Stopped) {
        regenerateQr();
        if (!m_qrMatrix.empty()) {
            drawQrCode(hdc, qrPanelX + 18, 115, 4);

            // Show encoded string below QR
            std::wstring qrLabel(m_qrLastEncoded.begin(), m_qrLastEncoded.end());
            SetTextColor(hdc, RGB(99, 102, 241));
            TextOutW(hdc, qrPanelX + 14, 115 + (int)m_qrMatrix.size() * 4 + 8,
                     qrLabel.c_str(), (int)qrLabel.size());
        }
    } else {
        // Show placeholder when receiver is stopped
        SetTextColor(hdc, RGB(71, 85, 105));
        TextOutW(hdc, qrPanelX + 18, 200, L"Start receiver", 14);
        TextOutW(hdc, qrPanelX + 18, 220, L"to show QR", 10);
    }

    // Blit to screen
    BitBlt(hdcWindow, 0, 0, width, height, hdc, 0, 0, SRCCOPY);

    SelectObject(hdc, oldBmp);
    DeleteObject(hbm);
    DeleteDC(hdc);

    EndPaint(hWnd, &ps);
}

void MainWindow::drawStick(HDC hdc, int centerX, int centerY, int radius, int16_t x, int16_t y, const wchar_t* label) {
    // Outer Circle
    HPEN penRing = CreatePen(PS_SOLID, 2, RGB(51, 65, 85));
    HBRUSH brBase = CreateSolidBrush(RGB(30, 41, 59));
    HPEN oldPen = (HPEN)SelectObject(hdc, penRing);
    HBRUSH oldBr = (HBRUSH)SelectObject(hdc, brBase);

    Ellipse(hdc, centerX - radius, centerY - radius, centerX + radius, centerY + radius);

    // Crosshairs
    MoveToEx(hdc, centerX - radius + 10, centerY, nullptr);
    LineTo(hdc, centerX + radius - 10, centerY);
    MoveToEx(hdc, centerX, centerY - radius + 10, nullptr);
    LineTo(hdc, centerX, centerY + radius - 10);

    // Thumb position dot: Android Y is positive Up, screen is positive Down
    int dotX = centerX + static_cast<int>((x * (radius - 14)) / 32767);
    int dotY = centerY - static_cast<int>((y * (radius - 14)) / 32767);

    HBRUSH brDot = CreateSolidBrush(RGB(99, 102, 241));
    SelectObject(hdc, brDot);
    Ellipse(hdc, dotX - 10, dotY - 10, dotX + 10, dotY + 10);
    DeleteObject(brDot);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBr);
    DeleteObject(penRing);
    DeleteObject(brBase);

    SetTextColor(hdc, RGB(148, 163, 184));
    TextOutW(hdc, centerX - 38, centerY + radius + 8, label, (int)wcslen(label));
}

void MainWindow::drawTrigger(HDC hdc, int x, int y, int width, int height, uint16_t val, const wchar_t* label) {
    // Gauge container
    HBRUSH brBg = CreateSolidBrush(RGB(30, 41, 59));
    RECT rc{x, y, x + width, y + height};
    FillRect(hdc, &rc, brBg);
    DeleteObject(brBg);

    // Gauge filled portion
    int fillW = static_cast<int>((static_cast<uint32_t>(val) * width) / 65535u);
    if (fillW > 0) {
        HBRUSH brFill = CreateSolidBrush(RGB(99, 102, 241));
        RECT rcFill{x, y, x + fillW, y + height};
        FillRect(hdc, &rcFill, brFill);
        DeleteObject(brFill);
    }

    HPEN penBorder = CreatePen(PS_SOLID, 1, RGB(71, 85, 105));
    HPEN oldPen = (HPEN)SelectObject(hdc, penBorder);
    HBRUSH oldBr = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, x, y, x + width, y + height);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBr);
    DeleteObject(penBorder);

    SetTextColor(hdc, RGB(255, 255, 255));
    TextOutW(hdc, x + 6, y + 4, label, (int)wcslen(label));
}

void MainWindow::drawButton(HDC hdc, int x, int y, int radius, bool pressed, COLORREF activeColor, const wchar_t* label) {
    COLORREF color = pressed ? activeColor : RGB(30, 41, 59);
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 2, pressed ? RGB(255, 255, 255) : RGB(71, 85, 105));

    HBRUSH oldBr = (HBRUSH)SelectObject(hdc, brush);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);

    Ellipse(hdc, x - radius, y - radius, x + radius, y + radius);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBr);
    DeleteObject(brush);
    DeleteObject(pen);

    SetTextColor(hdc, pressed ? RGB(255, 255, 255) : RGB(148, 163, 184));
    int textOffset = (radius > 16) ? 5 : 4;
    TextOutW(hdc, x - textOffset, y - 7, label, (int)wcslen(label));
}

void MainWindow::drawDpad(HDC hdc, int centerX, int centerY, uint8_t dpadMask) {
    const bool isUp    = (dpadMask & DPAD_UP) != 0;
    const bool isDown  = (dpadMask & DPAD_DOWN) != 0;
    const bool isLeft  = (dpadMask & DPAD_LEFT) != 0;
    const bool isRight = (dpadMask & DPAD_RIGHT) != 0;

    auto drawArm = [&](int x, int y, int w, int h, bool active) {
        HBRUSH br = CreateSolidBrush(active ? RGB(129, 140, 248) : RGB(30, 41, 59));
        RECT r{x, y, x + w, y + h};
        FillRect(hdc, &r, br);
        DeleteObject(br);
    };

    drawArm(centerX - 35, centerY - 12, 70, 24, isLeft || isRight);
    drawArm(centerX - 12, centerY - 35, 24, 70, isUp || isDown);

    SetTextColor(hdc, RGB(148, 163, 184));
    TextOutW(hdc, centerX - 18, centerY + 45, L"D-PAD", 5);
}

// ── QR Code helpers ──────────────────────────────────────────────────────────

std::string MainWindow::getLocalIpAddress() {
    // Walk IPv4 adapters, return first non-loopback UP address
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG bufLen = 0;
    GetAdaptersAddresses(AF_INET, flags, nullptr, nullptr, &bufLen);
    if (bufLen == 0) return "?.?.?.?";

    std::vector<uint8_t> buf(bufLen);
    auto* adapter = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data());
    if (GetAdaptersAddresses(AF_INET, flags, nullptr, adapter, &bufLen) != NO_ERROR)
        return "?.?.?.?";

    for (; adapter; adapter = adapter->Next) {
        if (adapter->OperStatus != IfOperStatusUp) continue;
        if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        for (auto* ua = adapter->FirstUnicastAddress; ua; ua = ua->Next) {
            auto* sa = reinterpret_cast<sockaddr_in*>(ua->Address.lpSockaddr);
            char ip[INET_ADDRSTRLEN]{};
            inet_ntop(AF_INET, &sa->sin_addr, ip, sizeof(ip));
            if (std::string(ip) != "127.0.0.1")
                return ip;
        }
    }
    return "?.?.?.?";
}

void MainWindow::regenerateQr() {
    uint16_t port = m_connectionManager.getReceiver().getPort();
    std::string ip  = getLocalIpAddress();
    std::string key = ip + ":" + std::to_string(port);
    if (key == m_qrLastEncoded) return; // nothing changed

    try {
        m_qrMatrix      = QrGen::encode(key);
        m_qrLastEncoded = key;
    } catch (...) {
        m_qrMatrix.clear();
    }
}

void MainWindow::drawQrCode(HDC hdc, int x, int y, int moduleSize) const {
    if (m_qrMatrix.empty()) return;
    int sz = (int)m_qrMatrix.size();

    // White background (quiet zone)
    int totalPx = sz * moduleSize;
    HBRUSH bgBrush = CreateSolidBrush(RGB(255, 255, 255));
    RECT bgRect{x - moduleSize, y - moduleSize,
                x + totalPx + moduleSize, y + totalPx + moduleSize};
    FillRect(hdc, &bgRect, bgBrush);
    DeleteObject(bgBrush);

    // Dark modules
    HBRUSH darkBrush = CreateSolidBrush(RGB(15, 23, 42)); // same as window bg
    for (int r = 0; r < sz; r++) {
        for (int c = 0; c < sz; c++) {
            if (m_qrMatrix[r][c]) {
                RECT rc{
                    x + c * moduleSize,
                    y + r * moduleSize,
                    x + c * moduleSize + moduleSize,
                    y + r * moduleSize + moduleSize
                };
                FillRect(hdc, &rc, darkBrush);
            }
        }
    }
    DeleteObject(darkBrush);
}

} // namespace GamepadReceiver
