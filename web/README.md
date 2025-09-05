 # 🎮 Joystick Web Application - Setup Guide
 
 A comprehensive guide for setting up the web-based joystick/gamepad input application using Emscripten, Node.js, and modern web technologies.
 
 ## 📋 Table of Contents
 
 - [Overview](#overview)
 - [System Requirements](#system-requirements)
 - [Emscripten Installation](#emscripten-installation)
 - [Environment Setup](#environment-setup)
 - [Project Setup](#project-setup)
 - [Building the Application](#building-the-application)
 - [Running the Application](#running-the-application)
 - [Troubleshooting](#troubleshooting)
 - [Development Workflow](#development-workflow)
 
 ## 🌟 Overview
 
 This web application transforms the original Windows-specific C++ joystick input application into a cross-platform web interface using:
 
 - **Emscripten**: Compiles C++14 code to WebAssembly
 - **Node.js**: Server runtime for the web application
 - **HTML5 Gamepad API**: Browser-native gamepad support
 - **Socket.IO**: Real-time communication
 - **Modern CSS**: Responsive visual interface with graphical gauges
 
 ## 💻 System Requirements
 
 ### Operating System Support
 - **Windows 10/11** (x64)
 - **macOS 10.15+** (Intel/Apple Silicon)
 - **Linux** (Ubuntu 18.04+, Debian 10+, or equivalent)
 
 ### Required Software
 - **Node.js**: v14.0.0 or higher
 - **npm**: v6.0.0 or higher
 - **Git**: Latest version
 - **Python**: 3.7+ (required by Emscripten)
 - **Modern web browser**: Chrome 80+, Firefox 75+, Safari 13+, Edge 80+
 
 ### Hardware Requirements
 - **RAM**: Minimum 4GB (8GB recommended for compilation)
 - **Storage**: 2GB free space for Emscripten SDK
 - **Gamepad**: Any modern USB/Bluetooth controller
 
 ## 🔧 Emscripten Installation
 
 ### Method 1: Using emsdk (Recommended)
 
 #### Step 1: Download and Install emsdk
 
 **Windows:**
 ```
 # Download emsdk
 git clone https://github.com/emscripten-core/emsdk.git
 cd emsdk
 
 # Install the latest SDK tools
 emsdk install latest
 
 # Activate the latest SDK tools
 emsdk activate latest
 
 # Add Emscripten to PATH for current session
 emsdk_env.bat
 ```
 
 **macOS/Linux:**
 ```
 # Download emsdk
 git clone https://github.com/emscripten-core/emsdk.git
 cd emsdk
 
 # Install the latest SDK tools
 ./emsdk install latest
 
 # Activate the latest SDK tools
 ./emsdk activate latest
 
 # Add Emscripten to PATH for current session
 source ./emsdk_env.sh
 ```
 
 #### Step 2: Verify Installation
 
 ```
 # Check Emscripten version
 emcc --version
 
 # Should output something like:
 # emcc (Emscripten gcc/clang-like replacement + linker emulating GNU ld) 3.1.50
 ```
 
 ### Method 2: Package Managers
 
 **macOS (Homebrew):**
 ```
 brew install emscripten
 ```
 
 **Ubuntu/Debian:**
 ```
 sudo apt update
 sudo apt install emscripten
 ```
 
 **Arch Linux:**
 ```
 sudo pacman -S emscripten
 ```
 
 ## 🌍 Environment Setup
 
 ### Permanent Environment Variables
 
 To avoid running activation scripts every time, set up permanent environment variables:
 
 #### Windows (PowerShell)
 
 1. Open PowerShell as Administrator
 2. Add to your PowerShell profile:
 
 ```
 # Edit PowerShell profile
 notepad $PROFILE
 
 # Add these lines to the profile:
 $env:EMSDK = "C:\path\to\emsdk"
 $env:PATH += ";$env:EMSDK;$env:EMSDK\upstream\emscripten"
 ```
 
 #### Windows (System Environment Variables)
 
 1. Open **System Properties** → **Advanced** → **Environment Variables**
 2. Add new **System Variables**:
    - `EMSDK`: `C:\path\to\emsdk`
    - Add to `PATH`: `%EMSDK%;%EMSDK%\upstream\emscripten`
 
 #### macOS/Linux (Bash/Zsh)
 
 Add to your shell profile (`~/.bashrc`, `~/.zshrc`, or `~/.profile`):
 
 ```
 # Emscripten Environment
 export EMSDK="/path/to/emsdk"
 export PATH="$EMSDK:$EMSDK/upstream/emscripten:$PATH"
 
 # Optional: Auto-activate emsdk on shell start
 source "$EMSDK/emsdk_env.sh" > /dev/null 2>&1
 ```
 
 ### Node.js Installation
 
 #### Windows
 1. Download from [nodejs.org](https://nodejs.org/)
 2. Run the installer
 3. Verify: `node --version` and `npm --version`
 
 #### macOS
 ```
 # Using Homebrew
 brew install node
 
 # Or download from nodejs.org
 ```
 
 #### Linux
 ```
 # Ubuntu/Debian
 curl -fsSL https://deb.nodesource.com/setup_lts.x | sudo -E bash -
 sudo apt-get install -y nodejs
 
 # Or using snap
 sudo snap install node --classic
 ```
 
 ## 🚀 Project Setup
 
 ### Step 1: Clone and Navigate
 
 ```
 git clone https://github.com/qwertybob1978/msi_gamecontroller_cli.git
 cd msi_gamecontroller_cli/web
 ```
 
 ### Step 2: Install Node.js Dependencies
 
 ```
 # Install all dependencies
 npm install
 
 # Or using yarn
 yarn install
 ```
 
 ### Step 3: Verify Environment
 
 ```
 # Check Node.js
 node --version
 
 # Check npm
 npm --version
 
 # Check Emscripten
 npm run check-emscripten
 ```
 
 ## 🔨 Building the Application
 
 ### Development Build
 
 ```
 # Full development build with debug symbols
 npm run build-debug
 
 # Or step by step:
 npm run build-wasm-debug
 npm run copy-wasm
 ```
 
 ### Production Build
 
 ```
 # Optimized production build
 npm run build
 
 # Or step by step:
 npm run build-wasm
 npm run copy-wasm
 ```
 
 ### Build Scripts Explained
 
 | Script | Description | Optimization | Debug Info |
 |--------|-------------|--------------|------------|
 | `build-wasm` | Production WebAssembly build | `-O2` | No |
 | `build-wasm-debug` | Debug WebAssembly build | `-O0` | Yes (`-g`) |
 | `copy-wasm` | Copy WASM files to public directory | N/A | N/A |
 | `build` | Full production build | High | No |
 | `build-debug` | Full debug build | None | Yes |
 
 ### Manual Compilation
 
 If you need to customize the build:
 
 ```
 # Basic compilation
 emcc joystick_web.cpp -o build/joystick.js \
   -s WASM=1 \
   -s EXPORTED_FUNCTIONS="['_ListGamepads','_StartStreaming','_StopStreaming','_UpdateGamepadState','_IsStreaming','_GetSelectedGamepad']" \
   -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']" \
   -s ALLOW_MEMORY_GROWTH=1 \
   -O2 \
   --bind
 
 # Copy to public directory
 cp build/joystick.js public/
 cp build/joystick.wasm public/
 ```
 
 ## 🏃‍♂️ Running the Application
 
 ### Development Mode
 
 ```
 # Start development server with auto-reload
 npm run dev
 
 # Server will start on http://localhost:3000
 ```
 
 ### Production Mode
 
 ```
 # Start production server
 npm start
 
 # Server will start on http://localhost:3000
 ```
 
 ### Custom Port
 
 ```
 # Set custom port
 PORT=8080 npm start
 
 # Or set in package.json config section
 ```
 
 ### Accessing the Application
 
 1. Open your web browser
 2. Navigate to `http://localhost:3000`
 3. Connect a gamepad to your computer
 4. Press any button on the gamepad to activate it
 5. Click "📡 Scan Gamepads" in the web interface
 6. Select your gamepad from the list
 7. Click "▶️ Start Visualization" to see real-time input
 
 ## 🔍 Troubleshooting
 
 ### Common Issues
 
 #### 1. Emscripten Not Found
 
 **Error:** `emcc: command not found`
 
 **Solutions:**
 ```
 # Re-run environment setup
 source /path/to/emsdk/emsdk_env.sh  # Linux/macOS
 # OR
 \/path/to/emsdk/emsdk_env.bat        # Windows
 
 # Check PATH
 echo $PATH  # Linux/macOS
 echo $env:PATH  # Windows PowerShell
 ```
 
 #### 2. Node.js Dependencies Issues
 
 **Error:** Module installation failures
 
 **Solutions:**
 ```
 # Clear npm cache
 npm cache clean --force
 
 # Delete node_modules and reinstall
 rm -rf node_modules package-lock.json
 npm install
 
 # Use specific Node.js version
 nvm use 18  # if using nvm
 ```
 
 #### 3. WebAssembly Compilation Errors
 
 **Error:** C++ compilation failures
 
 **Solutions:**
 ```
 # Verify C++ file syntax
 emcc --check joystick_web.cpp
 
 # Use debug build for more info
 npm run build-debug
 
 # Check Emscripten version compatibility
 emcc --version
 ```
 
 #### 4. Gamepad Not Detected
 
 **Error:** No gamepads found in browser
 
 **Solutions:**
 - **Firefox**: Enable `dom.gamepad.enabled` in `about:config`
 - **Chrome**: Ensure HTTPS or localhost
 - **All browsers**: Press buttons on gamepad to activate
 - **All browsers**: Try different USB port or restart browser
 
 #### 5. Port Already in Use
 
 **Error:** `EADDRINUSE: address already in use :::3000`
 
 **Solutions:**
 ```
 # Find process using port
 lsof -i :3000  # Linux/macOS
 netstat -ano | findstr :3000  # Windows
 
 # Kill process or use different port
 PORT=3001 npm start
 ```
 
 ### Debug Mode
 
 Enable verbose logging:
 
 ```
 # Debug Emscripten compilation
 EMCC_DEBUG=1 npm run build-debug
 
 # Debug Node.js server
 DEBUG=* npm run dev
 
 # Check browser console for JavaScript errors
 ```
 
 ## 🔄 Development Workflow
 
 ### Recommended Development Setup
 
 1. **Terminal 1**: File watching and auto-compilation
 ```
 # Watch for C++ changes and auto-rebuild
 npm run build-debug && npm run copy-wasm
 ```
 
 2. **Terminal 2**: Development server
 ```
 npm run dev
 ```
 
 3. **Browser**: Open Developer Tools for debugging
 
 ### Code Organization
 
 ```
 web/
 ├── joystick_web.cpp          # C++ source for WebAssembly
 ├── server.js                 # Node.js Express server
 ├── package.json              # Dependencies and scripts
 ├── build.sh                  # Build automation script
 ├── public/
 │   ├── index.html            # Main web interface
 │   ├── styles.css            # Enhanced CSS styles
 │   ├── joystick.js           # Generated WebAssembly module
 │   └── joystick.wasm         # Generated WebAssembly binary
 ├── build/                    # Build artifacts
 └── README_WEB.md             # This file
 ```
 
 ### Making Changes
 
 #### Modifying C++ Code:
 1. Edit `joystick_web.cpp`
 2. Run `npm run build-debug`
 3. Refresh browser
 
 #### Modifying Web Interface:
 1. Edit files in `public/`
 2. Refresh browser (no rebuild needed)
 
 #### Modifying Server:
 1. Edit `server.js`
 2. Server auto-restarts with `npm run dev`
 
 ### Performance Tips
 
 - Use **debug builds** during development for better error messages
 - Use **production builds** for deployment (smaller file size, better performance)
 - Enable browser caching for static assets in production
 - Consider using CDN for Socket.IO and other libraries
 
 ## 📚 Additional Resources
 
 ### Documentation
 - [Emscripten Documentation](https://emscripten.org/docs/)
 - [HTML5 Gamepad API](https://developer.mozilla.org/en-US/docs/Web/API/Gamepad_API)
 - [Node.js Documentation](https://nodejs.org/docs/)
 - [Socket.IO Documentation](https://socket.io/docs/)
 
 ### Useful Commands
 
 ```
 # Check WebAssembly module info
 wasm-objdump -h public/joystick.wasm
 
 # Analyze build size
 npm run build && ls -la public/joystick.*
 
 # Test gamepad detection
 # Open browser console and run:
 # navigator.getGamepads()
 ```
 
 ### Browser Compatibility
 
 | Browser | Version | Gamepad API | WebAssembly | Notes |
 |---------|---------|-------------|-------------|--------|
 | Chrome | 80+ | ✅ | ✅ | Full support |
 | Firefox | 75+ | ✅ | ✅ | Enable in about:config if needed |
 | Safari | 13+ | ✅ | ✅ | macOS/iOS support |
 | Edge | 80+ | ✅ | ✅ | Chromium-based |
 
 ---
 
 ## 🎯 Quick Reference
 
 ### Essential Commands
 ```
 # Setup
 npm install && npm run build
 
 # Development
 npm run dev
 
 # Production
 npm start
 
 # Rebuild WebAssembly
 npm run build
 
 # Clean build
 npm run clean && npm run build
 ```
 
 ### File Locations
 - **WebAssembly**: `public/joystick.js` & `public/joystick.wasm`
 - **Main Interface**: `public/index.html`
 - **Server**: `server.js`
 - **C++ Source**: `joystick_web.cpp`
 
 For additional help or issues, check the [project repository](https://github.com/qwertybob1978/msi_gamecontroller_cli) or open an issue.