# MSI Game Controller CLI (Proof of Concept)

This proof-of-concept console app lists game controllers and streams live input from a selected device. It merges devices from both XInput (Xbox family controllers) and DirectInput (generic HID gamepads/joysticks), then reads input using the appropriate API.

Important for MSI Claw users:
- The built-in controls must be in GAME MODE (not DESKTOP MODE) for inputs to be exposed. If inputs are not detected, switch to GAME MODE and try again.

## What it does

- No arguments: enumerates available controllers and prints a numbered list.
- One integer argument: selects a device by index and streams input to stdout until interrupted.

APIs used:
- XInput: polled input for Xbox 360/One/Series controllers (user indices 0–3).
- DirectInput: event-driven input via SetEventNotification and buffered data for generic devices.

To avoid duplicate entries, common XInput “proxy” devices exposed via DirectInput are filtered by name.

## Build

Requirements:
- Windows 10/11
- Visual Studio 2022 (C++14)
- Windows SDK

Libraries (already linked via `#pragma comment` in the source):
- xinput9_1_0.lib
- dinput8.lib
- dxguid.lib
- user32.lib
- ole32.lib

Steps:
1. Open the solution in Visual Studio 2022.
2. Select a suitable configuration (e.g., Release x64).
3. Build the solution.

## Usage

From a Developer Command Prompt or the build output directory:

- List devices:

JoystickInput.exe

- Stream input from a device by index:

JoystickInput.exe <deviceIndex>


Press Ctrl+C to stop streaming.

## Notes and limitations

- XInput supports up to 4 users (0–3) and must be polled; only state changes are printed to reduce spam.
- DirectInput devices are read using buffered, event-driven notifications.
- Button/axis layouts vary for DirectInput devices.
- This PoC prints to stdout only (no remapping, no rumble/FFB, no calibration).
- If a device disappears, the app attempts to re-acquire where possible.

## Troubleshooting

- MSI Claw: ensure the controls are in GAME MODE, not DESKTOP MODE.
- Reconnect the controller or try a different USB/Bluetooth path.
- Close other software that may have exclusive access to the device.
- Verify drivers are installed and the device appears in Windows Game Controllers.