#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <SDL2/SDL.h>
#include "fluid_cube.h"

#define MAX_PARTICLES 10000 // Maximum number of particles

// Particle structure
typedef struct {
  float x, y, z;
  float vx, vy, vz;
  float life;
} Particle;

// Particle system structure
typedef struct {
  Particle particles[MAX_PARTICLES];
  int numParticles;
} ParticleSystem;

// Function Declarations
void ParticleSystem_Init(ParticleSystem* system);
void ParticleSystem_AddParticle(ParticleSystem* system, float x, float y, float z, float vx, float vy, float vz);
void ParticleSystem_Update(ParticleSystem* system, FluidCube* fluid, float dt);
void ParticleSystem_Render(ParticleSystem* system, SDL_Renderer* renderer, int scale);

#endif //PARTICLE_SYSTEM_H