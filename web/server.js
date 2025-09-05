/**
 * @file
 * @brief Node.js server for the web-based joystick input application
 * @details Serves static files and provides WebSocket for real-time gamepad data
 */

const express = require('express');
const http = require('http');
const path = require('path');
const { Server } = require('socket.io');

const app = express();
const server = http.createServer(app);
const io = new Server(server);

const PORT = process.env.PORT || 3000;

// Serve static files
app.use(express.static(path.join(__dirname, 'public')));

// Serve the Emscripten compiled files
app.use('/wasm', express.static(path.join(__dirname, 'build')));

// Main route
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

// WebSocket connection handling
io.on('connection', (socket) => {
    console.log('Client connected:', socket.id);

    socket.on('gamepad-data', (data) => {
        // Broadcast gamepad data to all connected clients
        socket.broadcast.emit('gamepad-update', data);
    });

    socket.on('list-gamepads', () => {
        // Client requesting gamepad list
        socket.emit('gamepad-list-requested');
    });

    socket.on('start-streaming', (gamepadIndex) => {
        console.log(`Client ${socket.id} started streaming gamepad ${gamepadIndex}`);
        socket.broadcast.emit('streaming-started', { clientId: socket.id, gamepadIndex });
    });

    socket.on('stop-streaming', () => {
        console.log(`Client ${socket.id} stopped streaming`);
        socket.broadcast.emit('streaming-stopped', { clientId: socket.id });
    });

    socket.on('disconnect', () => {
        console.log('Client disconnected:', socket.id);
        socket.broadcast.emit('client-disconnected', { clientId: socket.id });
    });
});

server.listen(PORT, () => {
    console.log(`🎮 Joystick Web Server running on http://localhost:${PORT}`);
    console.log('📁 Serving files from:', path.join(__dirname, 'public'));
    console.log('🔧 WASM files served from:', path.join(__dirname, 'build'));
    console.log('\n🚀 Open your browser and connect a gamepad to get started!');
});

process.on('SIGINT', () => {
    console.log('\n👋 Shutting down server...');
    server.close(() => {
        console.log('✅ Server closed');
        process.exit(0);
    });
});