# OpenGL
# Terrain-Gen (OpenGL)

## Visual Studio Setup
Right click the project and open Properties. Then update these entries for both Debug/Release as needed:
1. Configuration Properties -> VC++ Directories -> Include Directories: Add `$(ProjectDir)Linking\include` and `$(ProjectDir)headers`
2. Configuration Properties -> VC++ Directories -> Library Directories: Add `$(ProjectDir)Linking\libs`
3. Configuration Properties -> C/C++ -> General -> Additional Include Directories: Add `$(ProjectDir)Linking\include;$(ProjectDir)headers`
4. Configuration Properties -> Linker -> General -> Additional Library Directories: Add `$(ProjectDir)Linking\libs`
5. Configuration Properties -> Linker -> Input -> Additional Dependencies: Add `opengl32.lib;glfw3.lib`

## Running on Windows Without Visual Studio (MSYS2 / MinGW-w64)
Install MSYS2 from https://www.msys2.org and open the MSYS2 MinGW 64-bit shell.

Install packages:
```bash
pacman -Syu
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-glfw mingw-w64-x86_64-glm
```

### 1. Create a build directory
```bash
mkdir -p build
```

### 2. Build the project
From the project root run:
```bash
g++ -std=c++17 TerrainGen.cpp Linking/glad.c -I./Linking/include -I./headers -o build/TerrainGen.exe -lglfw3 -lopengl32 -lgdi32
```

### 3. Run the application
```bash
./build/TerrainGen.exe
```

If linking fails, add extra Windows system libs:
```bash
g++ -std=c++17 TerrainGen.cpp Linking/glad.c -I./Linking/include -I./headers -o build/TerrainGen.exe -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32 -lwinmm
```

## Running on Linux
Build uses `Linking/glad.c` and the vendored headers in `Linking/include`. The project headers now live in `headers/`.

### 1. Install required packages (example: Ubuntu)
```bash
sudo apt update
sudo apt install build-essential g++ libglfw3-dev libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev libxxf86vm-dev libxcursor-dev libxinerama-dev
```

### 2. Create a build directory
```bash
mkdir -p build
```

### 3. Build the project
From the project root run:
```bash
g++ -std=c++17 TerrainGen.cpp Linking/glad.c \
  -I./Linking/include -I./headers \
  -o build/TerrainGen \
  -lglfw -lGL -ldl -pthread
```

If additional X11 libs are needed, append:
`-lX11 -lXrandr -lXi -lXxf86vm -lXcursor -lXinerama`

### 4. Run the application
```bash
./build/TerrainGen
```

The current `main()` renders a 10x10 wireframe grid. Close the window via the window controls or by pressing ESC.

## Notes
- Header files were moved into the `headers/` folder; build commands and IDE include paths above reference that.
- The project now uses `vertex_core.glsl` and `fragment_core.glsl` for rendering. These files must be present in the same directory as the executable or the working directory.
- An orbit camera is implemented: use Left Mouse Button to rotate and Scroll Wheel to zoom.

## Project Description
This project demonstrates a modern OpenGL setup using GLFW + GLAD and provides a starting point for rendering and camera code. The repository also contains placeholder shader and model code that can be re-enabled later if you want to load shaders or models.

### Building and Running Summary
- Visual Studio: update include/library directories to add `Linking\include`, `Linking\libs`, and `headers`.
- MSYS2/MinGW (Windows): compile `TerrainGen.cpp` with `Linking/glad.c` and include `headers`.
- Linux: compile `TerrainGen.cpp` with `Linking/glad.c` and include `headers`.

If you'd like, I can also add a small shader-less triangle render to verify GPU rendering beyond a blank clear color.
The application creates a window displaying the 3D plane model with dynamic lighting and interactive camera controls, serving as a foundation for more complex 3D game development projects.
