#ifndef FLUID_CUBE_H
#define FLUID_CUBE_H

#include "./coloring.h"

struct FluidCube {
    int sizeX;
    int sizeY;
    int sizeZ;
    float dt;
    float diff;
    float visc;

    float *s;
    float *density;

    float *Vx;
    float *Vy;
    float *Vz;

    float *Vx0;
    float *Vy0;
    float *Vz0;
};
typedef struct FluidCube FluidCube;

FluidCube *FluidCubeCreate(int sizeX, int sizeY, int sizeZ, float diffusion, float viscosity, float dt);
void FluidCubeFree(FluidCube *cube);
void FluidCubeAddDensity(FluidCube *cube, int x, int y, int z, float amount);
void FluidCubeAddVelocity(FluidCube *cube, int x, int y, int z, float amtX, float amtY, float amtZ);
void FluidCubeStep(FluidCube *cube);

void SDL_ExitWithError(const char *message);

#endif // FLUID_CUBE_H