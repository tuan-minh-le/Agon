# Agon

A multiplayer first-person 3D apartment exploration game built with C++ and OpenGL using the CGP graphics library.

## Overview

Agon is a 3D interactive application that features:
- **First-person exploration** of a realistic apartment environment
- **Multiplayer support** with real-time networking via WebSockets
- **User authentication** system with login/registration
- **Weapon mechanics** including shooting and reloading
- **Realistic physics** with collision detection and jumping
- **3D apartment layout** with multiple rooms (living room, bedroom, bathroom)

## Features

### 🎮 Core Gameplay
- **First-Person Movement**: WASD controls with mouse look
- **Jumping & Physics**: Space to jump with gravity simulation
- **Collision Detection**: Realistic wall and object collision
- **Weapon System**: Shooting mechanics with ammunition management
- **Room-based Multiplayer**: Join specific game rooms with other players

### 🏠 Environment
- **Multi-room Apartment**: Fully modeled 3-room apartment layout
- **Textured Surfaces**: Realistic floor, wall, and ceiling textures
- **Interactive Environment**: Collision-aware movement throughout the space

### 🌐 Networking & Authentication
- **User Login System**: Secure authentication with email/password
- **Real-time Communication**: WebSocket-based multiplayer networking
- **Room Management**: Join specific game rooms by ID
- **Token-based Security**: JWT authentication for secure connections

### 🎯 Controls
- **Movement**: `W/A/S/D` - Move forward/left/backward/right
- **Look Around**: Mouse movement for camera control
- **Jump**: `Space` - Jump with physics
- **Sprint**: `Shift` + movement - Faster movement speed
- **Shoot**: Left mouse button - Fire weapon
- **Reload**: `R` - Reload ammunition
- **Toggle Camera**: `\`` (backtick) - Switch between FPS and orbit camera
- **Exit FPS Mode**: `Escape` - Return to orbit camera mode

## Technical Architecture

### Graphics & Rendering
- **CGP Library**: Built on the CGP (Computer Graphics Programming) framework
- **OpenGL**: Modern OpenGL rendering pipeline
- **Mesh System**: Efficient 3D model loading and rendering
- **Texture Mapping**: Multi-texture support for realistic surfaces
- **Camera Systems**: Dual camera modes (FPS and orbit)

### Physics & Collision
- **AABB Collision**: Axis-Aligned Bounding Box collision detection
- **Smooth Movement**: Interpolated movement with acceleration/deceleration
- **Wall Sliding**: Smooth collision response with wall sliding
- **Gravity System**: Realistic jumping and falling mechanics

### Networking Stack
- **HTTP Client**: RESTful API communication for authentication
- **WebSocket Client**: Real-time bidirectional communication
- **Boost.Beast**: High-performance C++ networking library
- **JSON Processing**: Modern JSON parsing with nlohmann/json

## Project Structure

```
Agon/
├── src/
│   ├── main.cpp              # Application entry point
│   ├── scene.cpp/hpp         # Main scene management
│   ├── environment.cpp/hpp   # Graphics environment setup
│   ├── player.cpp/hpp        # Player controller and movement
│   ├── apartment.cpp/hpp     # 3D apartment environment
│   ├── weapon.cpp/hpp        # Weapon mechanics
│   └── login/                # Authentication system
│       ├── login_ui.cpp/hpp  # Login interface
│       ├── api_service.cpp/hpp     # HTTP API client
│       └── websocket_service.cpp/hpp # WebSocket client
├── assets/                   # Game assets
│   ├── floor.jpg            # Floor textures
│   ├── wall.jpg             # Wall textures
│   ├── ceiling.jpg          # Ceiling textures
│   └── man.obj              # 3D character model
└── shaders/                 # GLSL shaders
    ├── mesh/                # Mesh rendering shaders
    └── single_color/        # Basic color shaders
```

## Dependencies

### Core Libraries
- **CGP**: Computer Graphics Programming framework
- **OpenGL**: 3D graphics rendering
- **GLFW**: Window management and input handling
- **ImGui**: Immediate mode GUI library

### Networking
- **Boost.Beast**: WebSocket and HTTP client library
- **Boost.Asio**: Asynchronous I/O operations
- **nlohmann/json**: Modern C++ JSON library
- **httplib**: Lightweight HTTP/HTTPS library

### Asset Loading
- **Assimp**: 3D model loading library
- **STB Image**: Image loading for textures

## Building the Project

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential cmake
sudo apt install libglfw3-dev libglew-dev
sudo apt install libboost-all-dev
sudo apt install libassimp-dev

# macOS (with Homebrew)
brew install cmake glfw glew boost assimp

# Windows
# Install Visual Studio with C++ tools
# Install vcpkg for dependency management
```

### Compilation
```bash
# Clone the repository
git clone <repository-url>
cd Agon

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build the project
make -j$(nproc)

# Run the executable
./Agon
```

## Server Setup

The game requires a backend server for authentication and multiplayer functionality:

### Server Requirements
- **Authentication API**: JWT-based login system
- **WebSocket Server**: Real-time communication endpoint
- **Room Management**: Handle multiple game rooms

### Server Endpoints
- `POST /api/auth/login` - User authentication
- `GET /api/auth/me` - Get user information
- `WS /ws?token=<jwt>&roomId=<room>` - WebSocket connection

### Development Server
For development, update the server URLs in:
- `src/login/api_service.hpp` - Update `base_url`
- `src/scene.cpp` - Update WebSocket connection URL

## Configuration

### Graphics Settings
Modify `src/environment.cpp` for display settings:
```cpp
float project::gui_scale = 1.0f;          // UI scaling
bool project::fps_limiting = false;        // FPS cap toggle
float project::fps_max = 120.0f;          // Maximum FPS
bool project::vsync = false;              // V-Sync toggle
```

### Player Settings
Adjust player mechanics in `src/player.cpp`:
```cpp
movement_speed = 6.0f;       // Base movement speed
acceleration = 15.0f;        // Movement acceleration
max_velocity = 6.0f;         // Maximum velocity
gravity = 11.0f;             // Gravity strength
jumpForce = 4.5f;            // Jump power
collision_radius = 1.2f;     // Player collision size
```

### Weapon Configuration
Modify weapon properties in `src/weapon.cpp`:
```cpp
maxBullet = 30;              // Magazine capacity
totalAmmo = 120;             // Total ammunition
fireRate = 0.1f;             // Time between shots
reloadTime = 2.0f;           // Reload duration
bulletDamage = 10;           // Damage per bullet
```

## Gameplay

### Single Player Mode
- Enter "admin" as email to skip authentication
- Explore the apartment in first-person mode
- Practice shooting and movement mechanics

### Multiplayer Mode
1. **Login**: Enter valid credentials and room ID
2. **Connect**: Automatic WebSocket connection to game server
3. **Play**: Interact with other players in real-time
4. **Chat**: Use the integrated chat system (UI in development)

### Room Layout
The apartment features:
- **Living Room**: Main central area
- **Bedroom**: Private room with wall separation
- **Bathroom**: Smaller enclosed space
- **Realistic Layout**: Walls, doors, and proper room divisions

## Development

### Adding New Features
1. **New Weapons**: Extend the `Weapon` class with different types
2. **More Rooms**: Modify `apartment.cpp` to add room layouts
3. **Player Models**: Add animated character models
4. **Sound System**: Integrate audio for immersive experience
5. **Advanced Physics**: Add object interaction and physics

### Debug Features
- Press `Shift+V` to display camera debug information
- Use orbit camera mode for scene overview
- ImGui panels provide real-time debugging information

## Troubleshooting

### Common Issues

**Build Errors**:
- Ensure all dependencies are properly installed
- Check CMake configuration for missing libraries
- Verify C++17 compiler support

**Connection Issues**:
- Verify server is running and accessible
- Check firewall settings for WebSocket connections
- Ensure correct server URLs in configuration

**Graphics Problems**:
- Update graphics drivers
- Check OpenGL version compatibility
- Verify texture files are present in assets/

**Performance Issues**:
- Enable FPS limiting in settings
- Reduce texture resolution if needed
- Check for memory leaks in debug mode

## Contributing

1. **Fork** the repository
2. **Create** a feature branch
3. **Implement** your changes with proper testing
4. **Document** new features and APIs
5. **Submit** a pull request with detailed description

### Code Style
- Follow existing C++ conventions
- Use meaningful variable names
- Add comments for complex algorithms
- Keep functions focused and modular

## License

[Specify your license here]

## Acknowledgments

- **CGP Library**: Graphics framework foundation
- **Boost Libraries**: Networking infrastructure
- **ImGui**: User interface framework
- **Community**: Contributors and testers

---

**Note**: This project is actively developed. Features and APIs may change between versions. Check the latest documentation for updates.