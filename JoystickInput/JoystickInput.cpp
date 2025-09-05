/**
 * @file
 * @brief Lists game controllers and streams input for the selected device via XInput or DirectInput.
 * @details
 *   - Build: C++14, Windows desktop console
 *   - Links: xinput9_1_0.lib, dinput8.lib, dxguid.lib, user32.lib, ole32.lib
 *   - Behavior:
 *       - No args: list controllers with integer indices.
 *       - One int arg: select that controller and stream inputs.
 *   - API notes:
 *       - XInput devices (Xbox 360/One/Series) are polled; there is no event API in XInput.
 *       - DirectInput devices (generic USB gamepads/joysticks) are event-driven via SetEventNotification + buffered data.
 */

#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <Xinput.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>

#pragma comment(lib, "xinput9_1_0.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ole32.lib")

namespace {

    /**
     * @enum DeviceKind
     * @brief Identifies the API used to communicate with a controller.
     * @details
     *   @par Alternative options and trade-offs
     *   - XInput:
     *     - For Xbox family controllers and XInput-compatible pads.
     *     - Pros: Standardized layout (A/B/X/Y, triggers, sticks), vibration support, simple polling API.
     *     - Cons: Max 4 users (0..3), no event notifications (polling only), limited button/axis count.
     *   - DirectInput:
     *     - For generic HID gamepads/joysticks, flight sticks, wheels, etc.
     *     - Pros: Works with many legacy/non-XInput devices, supports more buttons/axes, event-driven via buffered data.
     *     - Cons: Layout varies by device, force feedback varies, some XInput devices also expose DI “proxy” devices (often filtered).
     */
    enum class DeviceKind {
        XInput,      //!< Use XInput API (Xbox controllers); polled reads; limited to 4 users.
        DirectInput  //!< Use DirectInput API (generic HID controllers); event-driven with buffered data.
    };

    /**
     * @brief Basic information about a discovered device in the merged list.
     */
    struct DeviceInfo {
        int index;                   //!< Zero-based stable index in the merged list presented to users.
        DeviceKind kind;             //!< Selected API for this device (see DeviceKind for alternatives).
        std::wstring name;           //!< Human-readable device name (UTF-16).
        // For XInput
        DWORD xinputUser = 0;        //!< XInput user index (0..3) when kind == DeviceKind::XInput.
        // For DirectInput
        GUID diGuid = GUID_NULL;     //!< DirectInput instance GUID when kind == DeviceKind::DirectInput.
    };

    /// Global run flag toggled by console control handler.
    std::atomic_bool g_Running{ true };
    /// Minimal hidden window required by DirectInput SetCooperativeLevel.
    HWND g_HiddenWnd = nullptr;

    /**
     * @brief Window procedure for the hidden helper window used by DirectInput.
     * @param hwnd Window handle.
     * @param msg Message.
     * @param wParam WPARAM.
     * @param lParam LPARAM.
     * @return LRESULT from processed message or DefWindowProc.
     * @remarks Only minimal handling is implemented; the window stays hidden.
     */
    LRESULT CALLBACK HiddenWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            return 0;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    /**
     * @brief Creates a hidden message-only window required by DirectInput cooperative level setup.
     * @return HWND of the created window, or nullptr on failure.
     * @note The class is registered once; subsequent calls reuse it.
     */
    HWND CreateHiddenWindow() {
        HINSTANCE hInst = GetModuleHandleW(nullptr);
        const wchar_t* kClassName = L"JoystickInputHiddenWnd";

        WNDCLASSW wc = {};
        wc.lpfnWndProc = HiddenWndProc;
        wc.hInstance = hInst;
        wc.lpszClassName = kClassName;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

        if (!RegisterClassW(&wc)) {
            // If already registered, continue
            if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
                return nullptr;
            }
        }

        HWND hwnd = CreateWindowExW(
            0, kClassName, L"Hidden", WS_OVERLAPPED,
            CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
            nullptr, nullptr, hInst, nullptr);

        if (hwnd) {
            ShowWindow(hwnd, SW_HIDE);
        }
        return hwnd;
    }

    /**
     * @brief Converts a UTF-16 wide string to UTF-8.
     * @param ws Source wide string.
     * @return UTF-8 encoded std::string.
     */
    std::string WToUtf8(const std::wstring& ws) {
        if (ws.empty()) return {};
        int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), nullptr, 0, nullptr, nullptr);
        std::string out(len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &out[0], len, nullptr, nullptr);
        return out;
    }

    /**
     * @brief Converts a UTF-8 string to UTF-16.
     * @param s Source UTF-8 string.
     * @return UTF-16 std::wstring.
     */
    std::wstring Utf8ToW(const std::string& s) {
        if (s.empty()) return {};
        int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
        std::wstring out(len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], len);
        return out;
    }

    /**
     * @brief Detects if an XInput user index is currently connected.
     * @param userIdx XInput user index in [0, 3].
     * @return true if connected; otherwise false.
     */
    bool IsXInputConnected(DWORD userIdx) {
        XINPUT_STATE state = {};
        DWORD res = XInputGetState(userIdx, &state);
        return (res == ERROR_SUCCESS);
    }

    /**
     * @brief Heuristic to skip likely XInput duplicates in DirectInput enumeration.
     * @param inst DirectInput device instance.
     * @return true if the device appears to be an XInput proxy; otherwise false.
     * @details Filters names containing "XInput", "(XBOX", or "IG_" (common DI proxies for XInput).
     */
    bool IsLikelyXInputDuplicate(const DIDEVICEINSTANCEW& inst) {
        std::wstring name(inst.tszProductName ? inst.tszProductName : L"");
        std::wstring lname = name;
        std::transform(lname.begin(), lname.end(), lname.begin(), ::towlower);
        if (lname.find(L"xinput") != std::wstring::npos) return true;
        if (lname.find(L"(xbox") != std::wstring::npos) return true;
        // Many DI proxies for XInput include "IG_"
        if (lname.find(L"ig_") != std::wstring::npos) return true;
        return false;
    }

    /// Context passed to DirectInput device enumeration.
    struct DIEnumContext {
        IDirectInput8W* di = nullptr;                 //!< Owning DirectInput interface.
        std::vector<DeviceInfo>* out = nullptr;       //!< Output list to append devices to.
    };

    /**
     * @brief Callback for DirectInput device enumeration (game controllers only).
     * @param pdidInstance Device instance provided by DirectInput.
     * @param pContext Pointer to DIEnumContext used to append results.
     * @return DIENUM_CONTINUE to continue enumeration.
     * @note Devices that appear to be XInput proxies are filtered out.
     */
    BOOL CALLBACK EnumDIEnumDevicesCallback(const DIDEVICEINSTANCEW* pdidInstance, VOID* pContext) {
        auto* ctx = reinterpret_cast<DIEnumContext*>(pContext);
        if (!ctx || !ctx->out) return DIENUM_CONTINUE;

        if (IsLikelyXInputDuplicate(*pdidInstance)) {
            // Skip XInput proxies; XInput will cover those.
            return DIENUM_CONTINUE;
        }

        DeviceInfo dev;
        dev.kind = DeviceKind::DirectInput;
        dev.name = pdidInstance->tszProductName ? pdidInstance->tszProductName : L"DirectInput Device";
        dev.diGuid = pdidInstance->guidInstance;
        ctx->out->push_back(std::move(dev));
        return DIENUM_CONTINUE;
    }

    /**
     * @brief Enumerates available input devices using XInput (users 0..3) and DirectInput.
     * @return A merged list of DeviceInfo with stable indices.
     * @details
     *   - XInput users [0..3] are added if connected.
     *   - DirectInput devices are enumerated via DI8 and filtered to avoid XInput duplicates.
     */
    std::vector<DeviceInfo> EnumerateDevices() {
        std::vector<DeviceInfo> devices;

        // 1) XInput users 0..3
        for (DWORD i = 0; i < 4; ++i) {
            if (IsXInputConnected(i)) {
                DeviceInfo dev;
                dev.kind = DeviceKind::XInput;
                dev.xinputUser = i;
                dev.name = L"XInput Controller " + std::to_wstring(i);
                devices.push_back(std::move(dev));
            }
        }

        // 2) DirectInput devices
        IDirectInput8W* di = nullptr;
        if (SUCCEEDED(DirectInput8Create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8W, (void**)&di, nullptr))) {
            DIEnumContext ctx{ di, &devices };
            di->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumDIEnumDevicesCallback, &ctx, DIEDFL_ATTACHEDONLY);
            di->Release();
        }

        // Assign stable indices
        for (int i = 0; i < (int)devices.size(); ++i) {
            devices[i].index = i;
        }
        return devices;
    }

    /**
     * @brief Prints a compact representation of an XInput state line to stdout.
     * @param s XINPUT_STATE to print.
     */
    void PrintXInputState(const XINPUT_STATE& s) {
        const XINPUT_GAMEPAD& g = s.Gamepad;
        std::cout
            << "LX=" << std::setw(6) << (int)g.sThumbLX << "  "
            << "LY=" << std::setw(6) << (int)g.sThumbLY << "  "
            << "RX=" << std::setw(6) << (int)g.sThumbRX << "  "
            << "RY=" << std::setw(6) << (int)g.sThumbRY << "  "
            << "LT=" << std::setw(3) << (int)g.bLeftTrigger << "  "
            << "RT=" << std::setw(3) << (int)g.bRightTrigger << "  "
            << "Buttons=0x" << std::hex << std::setw(4) << std::setfill('0') << g.wButtons << std::dec << std::setfill(' ') << "  "
            << "DPad(U/D/L/R)="
            << ((g.wButtons & XINPUT_GAMEPAD_DPAD_UP) ? 1 : 0) << "/"
            << ((g.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) ? 1 : 0) << "/"
            << ((g.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) ? 1 : 0) << "/"
            << ((g.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) ? 1 : 0)
            << "\n";
    }

    /**
     * @brief Prints a compact representation of a DirectInput DIJOYSTATE2 line to stdout.
     * @param js DirectInput state structure.
     * @note For brevity only the first 32 buttons are printed.
     */
    void PrintDIState(const DIJOYSTATE2& js) {
        std::cout
            << "AXES: "
            << "lX=" << std::setw(6) << js.lX << " "
            << "lY=" << std::setw(6) << js.lY << " "
            << "lZ=" << std::setw(6) << js.lZ << " "
            << "lRx=" << std::setw(6) << js.lRx << " "
            << "lRy=" << std::setw(6) << js.lRy << " "
            << "lRz=" << std::setw(6) << js.lRz << " "
            << "S0=" << std::setw(6) << js.rglSlider[0] << " "
            << "S1=" << std::setw(6) << js.rglSlider[1] << " | ";

        std::cout << "POV: ";
        for (int i = 0; i < 4; ++i) {
            DWORD pov = js.rgdwPOV[i];
            if (pov == 0xFFFFFFFF) std::cout << "---- ";
            else std::cout << std::setw(4) << pov << " ";
        }
        std::cout << "| BTN: ";
        for (int i = 0; i < 32; ++i) { // print first 32 buttons for brevity
            std::cout << ((js.rgbButtons[i] & 0x80) ? '1' : '0');
        }
        std::cout << "\n";
    }

    /**
     * @brief Console control handler to gracefully stop streaming on Ctrl+C/Ctrl+Break.
     * @param ctrlType One of the CTRL_* console events.
     * @return TRUE if handled; FALSE otherwise.
     */
    BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType) {
        switch (ctrlType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
            g_Running.store(false);
            return TRUE;
        default:
            return FALSE;
        }
    }

    /**
     * @brief Polls and prints input for a given XInput controller until interrupted.
     * @param userIndex XInput user index [0..3].
     * @return 0 on graceful exit, non-zero on disconnects or errors.
     * @details Uses packet numbers to only print on state changes; sleeps briefly to reduce CPU usage.
     */
    int RunXInputReader(DWORD userIndex) {
        std::cout << "Reading XInput controller " << userIndex << " (Ctrl+C to stop)...\n";
        XINPUT_STATE prev = {};
        DWORD lastPacket = 0;

        while (g_Running.load()) {
            XINPUT_STATE st = {};
            DWORD res = XInputGetState(userIndex, &st);
            if (res != ERROR_SUCCESS) {
                std::cout << "Controller disconnected.\n";
                return 1;
            }

            if (st.dwPacketNumber != lastPacket) {
                lastPacket = st.dwPacketNumber;
                PrintXInputState(st);
            }

            // XInput is inherently polled; sleep briefly to reduce CPU.
            // Using packet number ensures we print only on state changes.
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        return 0;
    }

    /**
     * @brief Reads and prints input from a DirectInput device using event notification and buffered data.
     * @param guidInstance DirectInput device instance GUID.
     * @return 0 on success; non-zero error code on failure.
     * @details
     *   - Sets joystick data format (DIJOYSTATE2).
     *   - Uses non-exclusive, background cooperative level.
     *   - Enables buffered input and attaches an event for notifications.
     *   - Acquires the device and loops until interrupted, handling re-acquire on input loss.
     */
    int RunDirectInputReader(const GUID& guidInstance) {
        std::cout << "Reading DirectInput device (Ctrl+C to stop)...\n";

        IDirectInput8W* di = nullptr;
        if (FAILED(DirectInput8Create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8W, (void**)&di, nullptr))) {
            std::cerr << "DirectInput8Create failed.\n";
            return 2;
        }

        IDirectInputDevice8W* dev = nullptr;
        if (FAILED(di->CreateDevice(guidInstance, &dev, nullptr))) {
            std::cerr << "CreateDevice failed.\n";
            di->Release();
            return 3;
        }

        if (!g_HiddenWnd) {
            g_HiddenWnd = CreateHiddenWindow();
            if (!g_HiddenWnd) {
                std::cerr << "Failed to create hidden window for DirectInput.\n";
                dev->Release();
                di->Release();
                return 4;
            }
        }

        if (FAILED(dev->SetDataFormat(&c_dfDIJoystick2))) {
            std::cerr << "SetDataFormat failed.\n";
            dev->Release(); di->Release();
            return 5;
        }

        if (FAILED(dev->SetCooperativeLevel(g_HiddenWnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND))) {
            std::cerr << "SetCooperativeLevel failed.\n";
            dev->Release(); di->Release();
            return 6;
        }

        // Enable buffered data so we can get event notifications
        DIPROPDWORD dipdw;
        dipdw.diph.dwSize = sizeof(DIPROPDWORD);
        dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        dipdw.diph.dwObj = 0;
        dipdw.diph.dwHow = DIPH_DEVICE;
        dipdw.dwData = 64; // buffer size
        dev->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);

        HANDLE hEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr); // auto-reset
        if (!hEvent) {
            std::cerr << "CreateEvent failed.\n";
            dev->Release(); di->Release();
            return 7;
        }

        if (FAILED(dev->SetEventNotification(hEvent))) {
            std::cerr << "SetEventNotification failed.\n";
            CloseHandle(hEvent);
            dev->Release(); di->Release();
            return 8;
        }

        HRESULT hr = dev->Acquire();
        if (FAILED(hr)) {
            std::cerr << "Acquire failed.\n";
            dev->SetEventNotification(nullptr);
            CloseHandle(hEvent);
            dev->Release(); di->Release();
            return 9;
        }

        // Main event loop: wait for device event, then read state
        while (g_Running.load()) {
            DWORD wait = WaitForSingleObject(hEvent, 100);
            if (wait == WAIT_OBJECT_0) {
                // Drain buffered events (optional) to keep buffer fresh
                DIDEVICEOBJECTDATA data[64];
                DWORD dwItems = 64;
                while (true) {
                    dwItems = 64;
                    hr = dev->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), data, &dwItems, 0);
                    if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) {
                        dev->Acquire();
                        continue;
                    }
                    if (FAILED(hr) || dwItems == 0) break;
                    // We don't print per-event; we print the full current state below.
                }

                DIJOYSTATE2 js = {};
                hr = dev->GetDeviceState(sizeof(DIJOYSTATE2), &js);
                if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) {
                    dev->Acquire();
                    continue;
                }
                if (SUCCEEDED(hr)) {
                    PrintDIState(js);
                }
            }
            else if (wait == WAIT_TIMEOUT) {
                // Periodic check to handle disconnections
                DIJOYSTATE2 js = {};
                hr = dev->GetDeviceState(sizeof(DIJOYSTATE2), &js);
                if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) {
                    dev->Acquire();
                }
                else if (FAILED(hr)) {
                    std::cout << "Device disconnected or error.\n";
                    break;
                }
            }
            else {
                // WAIT_FAILED
                std::cerr << "WaitForSingleObject failed.\n";
                break;
            }
        }

        dev->Unacquire();
        dev->SetEventNotification(nullptr);
        CloseHandle(hEvent);
        dev->Release();
        di->Release();
        return 0;
    }

    /**
     * @brief Prints usage and lists all available devices with their indices.
     * @details The list merges XInput and DirectInput devices; XInput proxies in DirectInput are filtered.
     */
    void PrintUsageAndList() {
        std::cout << "Usage: JoystickInput <deviceIndex>\n";
        std::cout << "No argument: lists available devices with their integer index.\n\n";

        auto devices = EnumerateDevices();
        if (devices.empty()) {
            std::cout << "No game controllers detected.\n";
            return;
        }

        std::cout << "Available devices:\n";
        for (const auto& d : devices) {
            std::cout << "  [" << d.index << "] "
                << (d.kind == DeviceKind::XInput ? "XInput   " : "DirectInp")
                << "  " << WToUtf8(d.name);
            if (d.kind == DeviceKind::XInput) {
                std::cout << " (user=" << d.xinputUser << ")";
            }
            std::cout << "\n";
        }
    }

} // namespace

/**
 * @brief Program entry point.
 * @param argc Argument count.
 * @param argv Argument vector; expects an optional device index.
 * @return Process exit code.
 * @details
 *   - Without arguments: prints usage and available devices.
 *   - With a valid index: starts streaming input using the appropriate API.
 */
int main(int argc, char* argv[]) {
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    if (!g_HiddenWnd) {
        g_HiddenWnd = CreateHiddenWindow(); // prepare for DI usage if needed
    }

    if (argc < 2) {
        PrintUsageAndList();
        return 0;
    }

    int selectedIndex = -1;
    try {
        selectedIndex = std::stoi(argv[1]);
    }
    catch (...) {
        std::cerr << "Invalid argument. Must be an integer device index.\n\n";
        PrintUsageAndList();
        return 1;
    }

    auto devices = EnumerateDevices();
    if (selectedIndex < 0 || selectedIndex >= (int)devices.size()) {
        std::cerr << "Device index out of range.\n\n";
        PrintUsageAndList();
        return 1;
    }

    const DeviceInfo& sel = devices[selectedIndex];
    std::cout << "Selected [" << sel.index << "] "
        << (sel.kind == DeviceKind::XInput ? "XInput   " : "DirectInp") << "  "
        << WToUtf8(sel.name) << "\n";

    if (sel.kind == DeviceKind::XInput) {
        return RunXInputReader(sel.xinputUser);
    }
    else {
        // Initialize COM for safety with some DI providers
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        int rc = RunDirectInputReader(sel.diGuid);
        CoUninitialize();
        return rc;
    }
}