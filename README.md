# Flipping is Hard - Practice Trainer

A Unity IL2CPP trainer for "Flipping is Hard Demo" that enables position saving/restoring for speedrun practice. Perfect for mastering difficult sections without restarting.

![Trainer Overlay](https://img.shields.io/badge/Status-Working-brightgreen) ![Platform](https://img.shields.io/badge/Platform-Windows-blue) ![License](https://img.shields.io/badge/License-MIT-green)

## ✨ Features
- **📌 Position Save/Restore** - Save any position with `Shift+R`, teleport back with `R`
- **✈️ Fly Mode** - Toggle free-camera flight with `F` key
  - Camera-relative movement with WASD (follows where you look)
  - Vertical movement with Space/Ctrl
  - Speed boost with Shift (3x faster)
  - No gravity - stay in the air
- **🔄 Physics Reset** - Automatically zeroes velocity when teleporting (supports both 3D & 2D physics)
- **🎮 On-Screen Overlay** - Real-time HUD showing controls, fly mode status, and saved position
- **🔧 No Sound Effects** - Visual feedback only, no annoying beeps
- **🎯 Works with IL2CPP** - Uses Unity's IL2CPP runtime for maximum compatibility
- **⚡ Fast & Lightweight** - Minimal performance impact on the game

## 🚀 Quick Start

### 1. **Build the Trainer**
```bash
build.bat
```
This compiles both `trainer.dll` and `injector.exe` using Visual Studio 2022.

### 2. **Start the Game**
Launch "Flipping is Hard Demo.exe" and get into gameplay.

### 3. **Inject the Trainer**
```bash
injector.exe
```
The injector automatically finds the game process and loads the trainer.

### 4. **Use In-Game**
- **`Shift + R`** → Save current position & rotation
- **`R`** → Teleport to saved position (resets physics)
- **`F`** → Toggle Fly Mode ON/OFF
- **Fly Mode Controls:**
  - `W` / `S` → Move forward/backward (relative to camera)
  - `A` / `D` → Move left/right (relative to camera)
  - `Space` → Move up (world space)
  - `Ctrl` → Move down (world space)
  - **`Shift` (hold)** → Speed boost (3x faster)
- **`END`** → Unload trainer

## 📁 Project Structure
```
Trainer/
├── trainer.cpp          # Main trainer DLL (IL2CPP hooks + overlay)
├── injector.cpp         # DLL injector (process injection)
├── build.bat            # Build script for Visual Studio
├── README.md            # This file
└── .gitignore           # Excludes compiled binaries
```

## 🛠️ Technical Details

### How It Works
1. **IL2CPP Resolution** - Hooks into `GameAssembly.dll` to resolve Unity engine functions
2. **Player Detection** - Searches for player GameObject via tags (`"Player"`) and common names
3. **Camera Detection** - Finds main camera via `Camera.main`, tags, and common names for fly mode
4. **Direct Transform Access** - Uses Unity's internal `Transform::get_position_Injected`/`set_position_Injected` icalls
5. **Physics Integration** - Detects and resets both `Rigidbody` and `Rigidbody2D` velocity
6. **Fly Mode System** - Camera-relative movement with delta-time smoothing and physics override
7. **Overlay System** - Creates a transparent always-on-top window with GDI+ rendering

### Key Components
- **Fly Mode Handler** - Camera-relative movement system with WASD controls following camera direction
- **Camera Finder** - Aggressive camera detection via multiple methods (Camera.main, tags, names)
- **Overlay Thread** - Separate thread for HUD rendering (350×180px, top-left corner)
- **Thread-Safe Logging** - Console output with file logging to `trainer_log.txt`
- **Smart Caching** - Player and camera references cached to minimize performance impact
- **Error Handling** - Graceful failure if game isn't ready or player not found
- **Clean Unload** - Proper cleanup on `END` key press

## 📋 Requirements
- **Windows 10/11** (x64)
- **Visual Studio 2022** (or any C++17 compiler)
- **Game**: "Flipping is Hard Demo" (Unity IL2CPP build)
- **Administrator rights** (for process injection)

## 🎮 Overlay Preview
The overlay appears in the top-left corner with:
```
[Trainer]
[FLY MODE ACTIVE]              (when fly mode is on)
WASD/Space/Ctrl to move        (fly mode controls)
Shift+R  ->  Save position
R        ->  Teleport
F        ->  Toggle Fly Mode
```

## ⚠️ Notes & Limitations
- **Game Must Be Running** - Injector requires the game process to be active
- **Player GameObject Required** - Only works in scenes with a player object
- **Camera Detection** - Fly mode works best when main camera is found (searches via Camera.main, tags, and names)
- **Third-Person Games** - Fly mode is optimized for 3rd person camera movement
- **Demo Version** - Tested with "Flipping is Hard Demo" (may work with full version)
- **Anti-Cheat** - This is for single-player practice only
- **Source Code** - 100% open source, no obfuscation

## 🔧 Building from Source
1. Install Visual Studio 2022 with C++ development tools
2. Navigate to project folder
3. Run `build.bat` 

## 🤝 Contributing
Found a bug or have an improvement? Feel free to:
1. Fork the repository
2. Create a feature branch
3. Submit a pull request

## 📄 License
MIT License - See [LICENSE](LICENSE) file for details.

Free for personal, educational, and non-commercial use.

## 🙏 Credits
- **Game**: "Flipping is Hard" by [Elegant Horse Studios]
- **Development**: Assisted by AI (because I don't know how to code 😅)
- **Testing**: Community feedback welcome!

---
**Disclaimer**: This tool is for educational purposes and single-player practice only. Use responsibly.