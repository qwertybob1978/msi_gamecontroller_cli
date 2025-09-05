#!/bin/bash
# Build script for Emscripten compilation

echo "🔨 Building Joystick Web Application with Emscripten..."

# Check if Emscripten is installed
if ! command -v emcc &> /dev/null; then
    echo "❌ Emscripten not found. Please install Emscripten first."
    echo "   Visit: https://emscripten.org/docs/getting_started/downloads.html"
    exit 1
fi

# Create build directory
mkdir -p build
mkdir -p public

echo "📦 Compiling C++ to WebAssembly..."

# Compile with Emscripten
emcc joystick_web.cpp \
    -o build/joystick.js \
    -s WASM=1 \
    -s EXPORTED_FUNCTIONS="['_ListGamepads','_StartStreaming','_StopStreaming','_UpdateGamepadState','_IsStreaming','_GetSelectedGamepad']" \
    -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']" \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s USE_SDL=0 \
    -s DISABLE_EXCEPTION_CATCHING=0 \
    -O2 \
    --bind

# Copy WASM files to public directory for serving
cp build/joystick.js public/
cp build/joystick.wasm public/

if [ $? -eq 0 ]; then
    echo "✅ WebAssembly compilation successful!"
    echo "📁 Files generated:"
    echo "   - public/joystick.js"
    echo "   - public/joystick.wasm"
    echo ""
    echo "🚀 Now run: npm start"
else
    echo "❌ Compilation failed!"
    exit 1
fi