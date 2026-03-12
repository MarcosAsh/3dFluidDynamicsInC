# 3D Fluid Dynamics in C

GPU-accelerated wind tunnel that simulates particles flowing around a car model using OpenGL compute shaders and a 3D fluid grid. Built with C, SDL2, and GLEW.

## Features
- 3D car model rendering with OBJ loader and adjustable scaling.
- Compute-shader particle system (hundreds of thousands of particles) with basic collision modes.
- 3D fluid grid utilities (diffusion, projection, advection) ready for further CFD experimentation.
- Interactive camera controls, wind speed tweaks, and collision mode toggles for quick testing.

## Requirements
- CMake 3.30+
- A C compiler with C11 support
- OpenGL 4.3+ capable GPU/driver (for compute shaders)
- SDL2, SDL2_ttf, GLEW, OpenGL dev packages discoverable via `pkg-config`

Example packages on Debian/Ubuntu:
```bash
sudo apt-get install build-essential cmake pkg-config \
  libsdl2-dev libsdl2-ttf-dev libglew-dev mesa-common-dev
```

## Build
```bash
git clone https://github.com/MarcosAsh/3dFluidDynamicsInC.git
cd 3dFluidDynamicsInC
cmake -B cmake-build-debug -S .
cmake --build cmake-build-debug
```

Artifacts are placed in `cmake-build-debug/3d_fluid_simulation_car`.

## Run
From the repo root:
```bash
./cmake-build-debug/3d_fluid_simulation_car
```
Assets are expected in `assets/` (e.g., `assets/3d-files/car-model.obj` and `assets/fonts/Arial.ttf`).

## Controls
- `Mouse drag`: orbit camera
- `Mouse wheel`: zoom camera
- `W/S`: tilt camera up/down
- `A/D`: rotate around target
- `Q/E`: zoom in/out (step)
- `R`: reset camera
- `Up/Down`: increase/decrease wind speed
- `0`: disable collision
- `1`: AABB collision (fast)
- `2`: per-triangle collision (accurate)
- `Esc`: quit

## Visualization Modes
   - `V`: Cycle through modes
   - `3-9`: Select specific mode
   - `LEFT/RIGHT`: Adjust color sensitivity
   
## Project Layout
- `src/`: simulation loop, particle system, rendering glue, and fluid grid implementation.
- `shaders/`: compute and render shaders for particle updates and drawing.
- `assets/`: OBJ model and fonts used at runtime.
- `obj-file-loader/`: lightweight OBJ parser used to load the car mesh.
- `lib/`: headers for fluid cube, particles, OpenGL helpers, and rendering utilities.

## Troubleshooting
- Missing libraries: ensure `pkg-config` finds SDL2, SDL2_ttf, GLEW, and OpenGL dev packages.
- Black screen: verify your GPU/driver supports OpenGL 4.3+ (compute shaders).
- Model issues: confirm `assets/3d-files/car-model.obj` exists and paths are relative to the repo root.

## Contributing
Issues and pull requests are welcome. For major changes, open an issue first to discuss direction and requirements.

## License
MIT — see `LICENSE` for details.
