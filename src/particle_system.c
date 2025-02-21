#include "../lib/particle_system.h"
#include "../lib/fluid_cube.h"
#include "../lib/config.h"

#include <stdlib.h>
#include <math.h>
#include <omp.h>  // For parallelization


// Initialize the particle system
void ParticleSystem_Init(ParticleSystem* system) {
    system->numParticles = 0;
}

// Add a particle to the system
void ParticleSystem_AddParticle(ParticleSystem* system, float x, float y, float z, float vx, float vy, float vz) {
    if (system->numParticles < MAX_PARTICLES) {
        Particle* p = &system->particles[system->numParticles++];
        p->x = x;
        p->y = y;
        p->z = z;
        p->vx = vx;
        p->vy = vy;
        p->vz = vz;
        p->life = 1.0f;  // Start with full lifetime
    }
}

// Update particle positions and velocities based on the fluid simulation
void ParticleSystem_Update(ParticleSystem* system, FluidCube* fluid, float dt) {
    int sizeX = fluid->sizeX;
    int sizeY = fluid->sizeY;
    int sizeZ = fluid->sizeZ;
    float* Vx = fluid->Vx;
    float* Vy = fluid->Vy;
    float* Vz = fluid->Vz;

    // Parallelize particle updates using OpenMP
    #pragma omp parallel for
    for (int i = 0; i < system->numParticles; i++) {
        Particle* p = &system->particles[i];

        // Get the grid cell indices for the particle's position
        int ix = (int)p->x;
        int iy = (int)p->y;
        int iz = (int)p->z;

        // Ensure the particle is within the fluid grid bounds
        if (ix >= 0 && ix < sizeX && iy >= 0 && iy < sizeY && iz >= 0 && iz < sizeZ) {
            // Update particle velocity based on the fluid's velocity field
            int index = IX3D(ix, iy, iz, sizeX, sizeY);
            p->vx = Vx[index];
            p->vy = Vy[index];
            p->vz = Vz[index];
        } else {
            // If the particle is outside the grid, reset its velocity
            p->vx = 0;
            p->vy = 0;
            p->vz = 0;
        }

        // Update particle position based on velocity
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        p->z += p->vz * dt;

        // Decrease particle lifetime
        p->life -= 0.01f * dt;

        // Reset particles that go out of bounds or expire
        if (p->x < 0 || p->x >= sizeX || p->y < 0 || p->y >= sizeY || p->z < 0 || p->z >= sizeZ || p->life <= 0) {
            p->x = (float)(rand() % sizeX);
            p->y = (float)(rand() % sizeY);
            p->z = (float)(rand() % sizeZ);
            p->life = 1.0f;  // Reset lifetime
        }
    }
}

// Render particles on the screen
void ParticleSystem_Render(ParticleSystem* system, SDL_Renderer* renderer, int scale) {
    // Set particle color once (white with alpha based on lifetime)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < system->numParticles; i++) {
        Particle* p = &system->particles[i];

        // Map particle position to screen coordinates
        int screenX = (int)(p->x * scale);
        int screenY = (int)(p->y * scale);

        // Draw particle as a small rectangle (2x2 pixels)
        SDL_Rect rect = {screenX, screenY, 2, 2};
        Uint8 alpha = (Uint8)(p->life * 255);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
        SDL_RenderFillRect(renderer, &rect);
    }
}