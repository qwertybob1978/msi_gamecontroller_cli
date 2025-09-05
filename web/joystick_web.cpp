/**
 * @file
 * @brief Web-compatible joystick input using HTML5 Gamepad API via Emscripten
 * @details
 *   - Build: Emscripten C++14
 *   - Links: Uses Emscripten's HTML5 bindings for gamepad support
 *   - Behavior: Provides web interface for gamepad enumeration and input streaming
 */

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/bind.h>
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <cstring>

namespace {
    
    /**
     * @brief Information about a gamepad device detected via HTML5 Gamepad API
     */
    struct GamepadInfo {
        int index;                    //!< Gamepad index (0-based)
        std::string id;               //!< Gamepad identifier string
        std::string mapping;          //!< Mapping type ("standard" or "")
        bool connected;               //!< Connection status
        int numButtons;               //!< Number of buttons
        int numAxes;                  //!< Number of axes
    };

    /**
     * @brief Current state of a gamepad
     */
    struct GamepadState {
        int index;                    //!< Gamepad index
        std::vector<double> axes;     //!< Axis values (-1.0 to 1.0)
        std::vector<bool> buttons;    //!< Button states (pressed/released)
        std::vector<double> buttonValues; //!< Button analog values (0.0 to 1.0)
        double timestamp;             //!< Timestamp of last update
    };

    std::vector<GamepadInfo> g_gamepads;
    bool g_streaming = false;
    int g_selectedGamepad = -1;

    /**
     * @brief Converts gamepad state to a formatted string for display
     * @param state The gamepad state to format
     * @return Formatted string representation
     */
    std::string FormatGamepadState(const GamepadState& state) {
        std::string result = "Gamepad " + std::to_string(state.index) + ": ";
        
        // Axes
        result += "AXES: ";
        for (size_t i = 0; i < state.axes.size(); ++i) {
            result += "A" + std::to_string(i) + "=" + 
                     std::to_string(static_cast<int>(state.axes[i] * 1000) / 1000.0) + " ";
        }
        
        // Buttons
        result += "| BUTTONS: ";
        for (size_t i = 0; i < state.buttons.size(); ++i) {
            result += (state.buttons[i] ? "1" : "0");
        }
        
        return result;
    }

    /**
     * @brief Updates the list of available gamepads
     */
    void UpdateGamepadList() {
        g_gamepads.clear();
        
        int numGamepads = emscripten_sample_gamepad_data();
        
        for (int i = 0; i < numGamepads; ++i) {
            EmscriptenGamepadEvent gamepadState;
            EMSCRIPTEN_RESULT result = emscripten_get_gamepad_status(i, &gamepadState);
            
            if (result == EMSCRIPTEN_RESULT_SUCCESS && gamepadState.connected) {
                GamepadInfo info;
                info.index = i;
                info.id = std::string(gamepadState.id);
                info.mapping = std::string(gamepadState.mapping);
                info.connected = gamepadState.connected;
                info.numButtons = gamepadState.numButtons;
                info.numAxes = gamepadState.numAxes;
                
                g_gamepads.push_back(info);
            }
        }
    }

    /**
     * @brief Gets the current state of a specific gamepad
     * @param index Gamepad index
     * @return GamepadState structure with current values
     */
    GamepadState GetGamepadState(int index) {
        GamepadState state;
        state.index = index;
        
        EmscriptenGamepadEvent gamepadEvent;
        EMSCRIPTEN_RESULT result = emscripten_get_gamepad_status(index, &gamepadEvent);
        
        if (result == EMSCRIPTEN_RESULT_SUCCESS && gamepadEvent.connected) {
            state.timestamp = gamepadEvent.timestamp;
            
            // Copy axes
            for (int i = 0; i < gamepadEvent.numAxes; ++i) {
                state.axes.push_back(gamepadEvent.axis[i]);
            }
            
            // Copy buttons
            for (int i = 0; i < gamepadEvent.numButtons; ++i) {
                state.buttons.push_back(gamepadEvent.digitalButton[i] != 0);
                state.buttonValues.push_back(gamepadEvent.analogButton[i]);
            }
        }
        
        return state;
    }
}

// C++ functions exposed to JavaScript
extern "C" {
    
    /**
     * @brief Lists all available gamepads
     * @return Number of gamepads found
     */
    EMSCRIPTEN_KEEPALIVE
    int ListGamepads() {
        UpdateGamepadList();
        
        std::cout << "Available gamepads:\n";
        for (const auto& gamepad : g_gamepads) {
            std::cout << "  [" << gamepad.index << "] " 
                     << gamepad.id << " (" << gamepad.mapping << ") "
                     << "Buttons: " << gamepad.numButtons << " "
                     << "Axes: " << gamepad.numAxes << "\n";
        }
        
        if (g_gamepads.empty()) {
            std::cout << "No gamepads detected. Make sure a gamepad is connected and press any button on it.\n";
        }
        
        return static_cast<int>(g_gamepads.size());
    }

    /**
     * @brief Starts streaming input from a specific gamepad
     * @param gamepadIndex Index of the gamepad to stream from
     * @return 1 on success, 0 on failure
     */
    EMSCRIPTEN_KEEPALIVE
    int StartStreaming(int gamepadIndex) {
        UpdateGamepadList();
        
        if (gamepadIndex < 0 || gamepadIndex >= static_cast<int>(g_gamepads.size())) {
            std::cout << "Invalid gamepad index: " << gamepadIndex << "\n";
            return 0;
        }
        
        g_selectedGamepad = gamepadIndex;
        g_streaming = true;
        
        std::cout << "Starting to stream input from gamepad " << gamepadIndex 
                 << " (" << g_gamepads[gamepadIndex].id << ")\n";
        std::cout << "Press ESC or call StopStreaming() to stop.\n";
        
        return 1;
    }

    /**
     * @brief Stops streaming gamepad input
     */
    EMSCRIPTEN_KEEPALIVE
    void StopStreaming() {
        g_streaming = false;
        g_selectedGamepad = -1;
        std::cout << "Stopped streaming gamepad input.\n";
    }

    /**
     * @brief Updates and prints gamepad state (called from JavaScript animation loop)
     */
    EMSCRIPTEN_KEEPALIVE
    void UpdateGamepadState() {
        if (!g_streaming || g_selectedGamepad < 0) {
            return;
        }
        
        // Sample gamepad data
        emscripten_sample_gamepad_data();
        
        GamepadState state = GetGamepadState(g_selectedGamepad);
        
        // Only print if we have valid data
        if (!state.axes.empty() || !state.buttons.empty()) {
            std::cout << FormatGamepadState(state) << "\n";
        }
    }

    /**
     * @brief Checks if currently streaming
     * @return 1 if streaming, 0 otherwise
     */
    EMSCRIPTEN_KEEPALIVE
    int IsStreaming() {
        return g_streaming ? 1 : 0;
    }

    /**
     * @brief Gets the currently selected gamepad index
     * @return Gamepad index or -1 if none selected
     */
    EMSCRIPTEN_KEEPALIVE
    int GetSelectedGamepad() {
        return g_selectedGamepad;
    }
}

// Main function - not used in web context but required for compilation
int main() {
    std::cout << "Joystick Web Module initialized. Use JavaScript interface to interact.\n";
    return 0;
}