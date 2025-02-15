cmake_minimum_required(VERSION 3.30)
project(3d_fluid_simulation_car C)

set(CMAKE_C_STANDARD 11)

add_executable(3d_fluid_simulation_car main.c
        lib/fluid_cube.h
        lib/coloring.h
        src/coloring.c
        src/fluid_cube.c
        obj-file-loader/lib/model_loader.h
        obj-file-loader/src/model_laoder.c
        obj-file-loader/src/render_model.c
        obj-file-loader/lib/render_model.h
)
