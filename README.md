# Windows Virtual Gamepad Receiver (C++17)

A native, ultra-low latency C++17 Windows application that receives real-time (60–120Hz) gamepad packets from an Android phone over local Wi-Fi via UDP and feeds them directly into Windows and PC games as a native Xbox 360 controller.

---

## 🚀 1. Quick Overview

- **Input Latency**: Sub-millisecond internal processing, zero heap allocations in the critical path.
- **Protocol**: 33-byte Little-Endian fixed binary packet matching the Android Virtual Gamepad.
- **Failsafe Watchdog**: Automatic neutral reset after 300ms if connection drops (no stuck buttons).
- **Virtual Controller Backend**: Dynamic integration with Nefarius ViGEmBus (Xbox 360 emulation). Recognized by Steam, Epic Games, Xbox App, RetroArch, and all XInput games.
- **Native Win32 GUI**: Double-buffered GDI dashboard with live thumbstick crosshairs, button indicators, and 15Hz telemetry decoupled from the 120Hz UDP loop.
- **System Tray Mode**: Runs seamlessly in the background.

---

## 🏛️ 2. Architecture & Data Flow

```
[ Android Kotlin Phone ]
        | 60-120Hz UDP (Local Wi-Fi)
        v
[ Winsock2 UDP Receiver ] (Dedicated blocking worker thread)
        | Fixed 33-byte packet buffer
        v
[ PacketDecoder ] (Explicit Little-Endian deserialization)
        |
        +---> Handshake / Heartbeat -> Immediate ACK reply
        |
        v
[ ControllerProcessor ] (Sequence monotonic check, out-of-order drop, failsafe)
        |
        v
[ Latest-State Buffer ] (Lock-free / atomic newest-state wins)
        |
        v
[ GamepadMapper ] (Maps raw inputs to XINPUT_GAMEPAD struct)
        |
        v
[ ViGEmClient.dll (ViGEmBus) ]
        |
        v
[ Windows Games / Steam / Game Controllers ]
```

---

## 📦 3. Binary Packet Protocol (33 Bytes, Little-Endian)

| Offset | Size | Type | Field Name | Windows Description |
|---|---|---|---|---|
| `0` | 2 | uint16_t | `magic` | `0x4750` ('GP' in ASCII) |
| `2` | 1 | uint8_t | `version` | `0x01` |
| `3` | 1 | uint8_t | `packetType` | `0x01` = State, `0x02` = Hello, `0x03` = Ack, `0x04` = Heartbeat |
| `4` | 4 | uint32_t | `sequence` | Incrementing packet sequence |
| `8` | 8 | int64_t | `timestampNs` | Monotonic timestamp from Android phone |
| `16` | 2 | int16_t | `leftThumbX` | $-32767$ to $+32767$ (`sThumbLX`) |
| `18` | 2 | int16_t | `leftThumbY` | $-32767$ to $+32767$ (`sThumbLY`, +Up) |
| `20` | 2 | int16_t | `rightThumbX` | $-32767$ to $+32767$ (`sThumbRX`) |
| `22` | 2 | int16_t | `rightThumbY` | $-32767$ to $+32767$ (`sThumbRY`, +Up) |
| `24` | 2 | uint16_t | `leftTrigger` | 0 to 65535 (Mapped to 0..255 `bLeftTrigger`) |
| `26` | 2 | uint16_t | `rightTrigger` | 0 to 65535 (Mapped to 0..255 `bRightTrigger`) |
| `28` | 4 | uint32_t | `buttons` | 32-bit digital button bitmask |
| `32` | 1 | uint8_t | `dpad` | 8-bit D-Pad bitmask |

### Button Flags:
- `1 << 0`: Button A
- `1 << 1`: Button B
- `1 << 2`: Button X
- `1 << 3`: Button Y
- `1 << 4`: Left Bumper (LB / L1)
- `1 << 5`: Right Bumper (RB / R1)
- `1 << 6`: Left Thumb Click (L3)
- `1 << 7`: Right Thumb Click (R3)
- `1 << 8`: START
- `1 << 9`: BACK / SELECT

---

## 🛠️ 4. How to Build

### Option A: Using CMake & Visual Studio 2022
1. Open PowerShell and navigate to the project directory:
   ```powershell
   cd C:\Users\sin\Desktop\WindowsGamepadReceiver
   cmake -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release
   ```
2. Executables will be placed in `build\Release\`:
   - `WindowsGamepadReceiver.exe` (Main Win32 App)
   - `ReceiverUnitTests.exe` (Unit Test Suite)
   - `TestSender.exe` (120Hz Test Packet Generator)

### Option B: Direct Compilation with MSVC (Visual Studio Developer Command Prompt)
```cmd
cl.exe /O2 /std:c++17 /EHsc /Fe:WindowsGamepadReceiver.exe src/main/main.cpp src/network/*.cpp src/controller/*.cpp src/gamepad/*.cpp src/connection/*.cpp src/diagnostics/*.cpp src/ui/*.cpp src/utils/*.cpp ws2_32.lib comctl32.lib
```

---

## 🎮 5. ViGEmBus Virtual Gamepad Setup

For PC games to detect the application as a real physical Xbox 360 controller:
1. Download and install the free, digitally signed **ViGEmBus driver** from Nefarius:
   👉 [https://github.com/nefarius/ViGEmBus/releases](https://github.com/nefarius/ViGEmBus/releases)
2. Run `ViGEmBus_Setup_x64.exe` and follow the on-screen prompts.
3. Once installed, `WindowsGamepadReceiver.exe` will automatically detect the driver on startup and create a virtual Xbox 360 controller recognized by all games!
*(Note: If ViGEmBus is not installed, the application automatically runs in Mock Mode so you can still visualize incoming inputs and test latency).*

---

## 🛡️ 6. Windows Firewall Configuration

When running for the first time, Windows Defender Firewall may prompt you to allow incoming UDP packets:
1. Click **Allow access** on Private networks.
2. Or manually allow UDP port 5000:
   ```powershell
   New-NetFirewallRule -DisplayName "Virtual Gamepad UDP" -Direction Inbound -LocalPort 5000 -Protocol UDP -Action Allow
   ```

---

## 🧪 7. Testing & Verification

### 1. Run Built-in Unit Tests
```powershell
.\ReceiverUnitTests.exe
```
Verifies exact byte unpacking matching Android test vectors, sequence wrap-around, out-of-order rejection, and 300ms failsafe timeout.

### 2. Run Local 120Hz Simulator (No Phone Required)
```powershell
.\TestSender.exe 127.0.0.1 5000
```
Sends circular joystick animations and button pulses directly to the receiver window to verify full 120Hz smoothness!

### 3. Verify in Windows Game Controller Settings
Press `Win + R`, type `joy.cpl`, and press Enter. You will see **Controller (XBOX 360 For Windows)** moving in real-time!
