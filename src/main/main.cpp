#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include "../connection/ConnectionManager.h"
#include "../ui/MainWindow.h"
#include "../utils/Logger.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow) {
    uint16_t port = 5000;
    bool headless = false;

    // Parse command line arguments
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv) {
        for (int i = 1; i < argc; ++i) {
            std::wstring arg = argv[i];
            if (arg == L"--port" && i + 1 < argc) {
                port = static_cast<uint16_t>(_wtoi(argv[++i]));
            } else if (arg == L"--headless") {
                headless = true;
            }
        }
        LocalFree(argv);
    }

    GamepadReceiver::ConnectionManager connectionManager;

    if (!connectionManager.start(port)) {
        MessageBoxW(
            nullptr,
            L"Failed to start UDP Gamepad Receiver. Ensure port 5000 is not blocked or in use.",
            L"Virtual Gamepad Error",
            MB_ICONERROR | MB_OK
        );
        return 1;
    }

    if (headless) {
        // Run as console / service loop without GUI
        LOG_INFO("Running in headless background mode on port " + std::to_string(port));
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return 0;
    }

    GamepadReceiver::MainWindow mainWindow(hInstance, connectionManager);
    if (!mainWindow.create(nCmdShow)) {
        MessageBoxW(
            nullptr,
            L"Failed to initialize Gamepad Receiver window.",
            L"Fatal Error",
            MB_ICONERROR | MB_OK
        );
        return 1;
    }

    return mainWindow.runMessageLoop();
}

// Fallback for standard main when compiled as console application
int main(int argc, char* argv[]) {
    return wWinMain(GetModuleHandleW(nullptr), nullptr, GetCommandLineW(), SW_SHOW);
}
