# Flipping is Hard - Practice Trainer

A simple trainer for the Unity game "Flipping is Hard Demo" that allows saving and restoring player positions for speedrun practice.

## Features
- **Save position**: Press `Shift + R` to save current player position and rotation
- **Restore position**: Press `R` to teleport back to saved position
- **Physics reset**: Automatically resets velocity when teleporting
- **On-screen overlay**: Shows controls and saved position status
- **Works with both 3D and 2D physics** (Rigidbody & Rigidbody2D)

## How to Use
1. **Build the trainer**:
   ```
   build.bat
   ```
   This will compile `trainer.dll` and `injector.exe`

2. **Start the game** ("Flipping is Hard Demo.exe")

3. **Inject the DLL**:
   ```
   injector.exe
   ```
   The injector will find the game process and load the trainer.

4. **In-game controls**:
   - `Shift + R` → Save current position
   - `R` → Teleport to saved position
   - `END` → Unload trainer

## Files
- `trainer.cpp` - Main trainer DLL source code
- `injector.cpp` - DLL injector source code  
- `build.bat` - Build script for Visual Studio 2022
- `trainer.dll` - Compiled trainer (output)
- `injector.exe` - Compiled injector (output)

## Requirements
- Visual Studio 2022 (or compatible C++ compiler)
- Windows 10/11
- Game: "Flipping is Hard Demo" (Unity IL2CPP build)

## How It Works
The trainer uses IL2CPP runtime inspection to:
1. Resolve Unity engine functions via `GameAssembly.dll`
2. Find the player GameObject by name/tag
3. Use direct Transform icalls for position/rotation manipulation
4. Reset physics velocity through Rigidbody components

## Notes
- The overlay appears in the top-left corner showing controls and saved position
- No sound effects - visual feedback only
- Works in gameplay scenes where the player GameObject exists
- Tested with the demo version of "Flipping is Hard"

## License
MIT License - free for personal and educational use.