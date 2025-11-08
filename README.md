# MiniAssignment – NetProbe

## Overview
A small cross-platform C++ tool to:
- Check remote IP reachability (via ping)
- Detect if local and remote IPs are in the same network
- Scan local ports and show status (Used/Idle)
- Optionally kill processes holding a port

## Features
- Cross-platform (Windows + Linux/WSL)
- Uses CMake for easy build
- Optional `--kill` flag to terminate port processes

---

## Build Instructions

### 🪟 Windows
1. Open project in Visual Studio
2. Select **Local Machine (x64-Debug)**
3. Build → Rebuild Solution
4. Run:
   ```powershell
   .\out\build\x64-Debug\MiniAssignment.exe 127.0.0.1 --ports 22,80,8080
