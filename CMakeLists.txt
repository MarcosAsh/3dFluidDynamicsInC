cmake_minimum_required(VERSION 3.30)
project(3d_fluid_simulation_car C)

set(CMAKE_C_STANDARD 11)

add_executable(3d_fluid_simulation_car src/main.c
        lib/fluid_cube.h
        lib/coloring.h
        src/coloring.c
        src/fluid_cube.c
        obj-file-loader/lib/model_loader.h
        obj-file-loader/lib/model_laoder.c
        src/render_model.c
        lib/render_model.h
        src/particle_system.c
        lib/particle_system.h
        lib/config.h
        lib/opengl_utils.h
        src/opengl_utils.c
)
