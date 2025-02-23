#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <SDL2/SDL.h>
#include "fluid_cube.h"
#include "config.h"

#define MAX_PARTICLES 10000 // Maximum number of particles
#define GRID_CELL_SIZE 10 // Size of each grid cell

// Particle structure
typedef struct {
  float x, y, z;    // Position
  float vx, vy, vz; // Velocity
  float life;       // Lifetime (0.0 to 1.0)
} Particle;

// Grid cell structure
typedef struct {
  Particle particles[MAX_PARTICLES / GRID_CELL_SIZE]; // Particles in this cell
  int count; // Number of particles in this cell
} GridCell;

// Particle system structure
typedef struct {
  Particle particles[MAX_PARTICLES]; // Array of particles
  int numParticles; // Current number of particles
  GridCell grid[GRID_CELL_SIZE][GRID_CELL_SIZE]; // Grid for spatial partitioning
} ParticleSystem;

// Function Declarations
void ParticleSystem_Init(ParticleSystem* system);
void ParticleSystem_AddParticle(ParticleSystem* system, float x, float y, float z, float vx, float vy, float vz);
void ParticleSystem_Update(ParticleSystem* system, FluidCube* fluid, float dt);
void ParticleSystem_Render(ParticleSystem* system, SDL_Renderer* renderer, int scale);

#endif //PARTICLE_SYSTEM_H