#include "../lib/particle_system.h"
#include "../lib/fluid_cube.h"
#include "../lib/config.h"

#include <stdlib.h>
#include <math.h>
#include <omp.h>  // For parallelization
#include <immintrin.h>  // For AVX intrinsics

// Initialize the particle system
void ParticleSystem_Init(ParticleSystem* system) {
    system->numParticles = 0;
    for (int i = 0; i < GRID_CELL_SIZE; i++) {
        for (int j = 0; j < GRID_CELL_SIZE; j++) {
            system->grid[i][j].count = 0;
        }
    }
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

        // Add particle to the grid
        int gridX = (int)(x / GRID_CELL_SIZE);
        int gridY = (int)(y / GRID_CELL_SIZE);
        if (gridX >= 0 && gridX < GRID_CELL_SIZE && gridY >= 0 && gridY < GRID_CELL_SIZE) {
            GridCell* cell = &system->grid[gridX][gridY];
            if (cell->count < MAX_PARTICLES / GRID_CELL_SIZE) {
                cell->particles[cell->count++] = *p;
            }
        }
    }
}

void ParticleSystem_Update(ParticleSystem* system, FluidCube* fluid, float dt) {
    int sizeX = fluid->sizeX;
    int sizeY = fluid->sizeY;
    int sizeZ = fluid->sizeZ;
    float* Vx = fluid->Vx;
    float* Vy = fluid->Vy;
    float* Vz = fluid->Vz;

    // Clear the grid
    for (int i = 0; i < GRID_CELL_SIZE; i++) {
        for (int j = 0; j < GRID_CELL_SIZE; j++) {
            system->grid[i][j].count = 0;
        }
    }

    // Update particles and reinsert them into the grid
    #pragma omp parallel for
    for (int i = 0; i < system->numParticles; i += 8) {
#ifdef __AVX__
        __m256 p_x = _mm256_loadu_ps(&system->particles[i].x);
        __m256 p_y = _mm256_loadu_ps(&system->particles[i].y);
        __m256 p_z = _mm256_loadu_ps(&system->particles[i].z);
        __m256 p_vx = _mm256_loadu_ps(&system->particles[i].vx);
        __m256 p_vy = _mm256_loadu_ps(&system->particles[i].vy);
        __m256 p_vz = _mm256_loadu_ps(&system->particles[i].vz);
        __m256 p_life = _mm256_loadu_ps(&system->particles[i].life);

        // Update particle positions and velocities (SIMD version)
        p_x = _mm256_add_ps(p_x, _mm256_mul_ps(p_vx, _mm256_set1_ps(dt)));
        p_y = _mm256_add_ps(p_y, _mm256_mul_ps(p_vy, _mm256_set1_ps(dt)));
        p_z = _mm256_add_ps(p_z, _mm256_mul_ps(p_vz, _mm256_set1_ps(dt)));
        p_life = _mm256_sub_ps(p_life, _mm256_set1_ps(0.01f * dt));

        // Store updated values
        _mm256_storeu_ps(&system->particles[i].x, p_x);
        _mm256_storeu_ps(&system->particles[i].y, p_y);
        _mm256_storeu_ps(&system->particles[i].z, p_z);
        _mm256_storeu_ps(&system->particles[i].life, p_life);

        // Reinsert particles into the grid (non-SIMD)
        for (int j = 0; j < 8; j++) {
            Particle* p = &system->particles[i + j];
            int gridX = (int)(p->x / GRID_CELL_SIZE);
            int gridY = (int)(p->y / GRID_CELL_SIZE);
            if (gridX >= 0 && gridX < GRID_CELL_SIZE && gridY >= 0 && gridY < GRID_CELL_SIZE) {
                GridCell* cell = &system->grid[gridX][gridY];
                if (cell->count < MAX_PARTICLES / GRID_CELL_SIZE) {
                    cell->particles[cell->count++] = *p;
                }
            }
        }
#else
        for (int j = 0; j < 8; j++) {
            Particle* p = &system->particles[i + j];
            p->x += p->vx * dt;
            p->y += p->vy * dt;
            p->z += p->vz * dt;
            p->life -= 0.01f * dt;

            int gridX = (int)(p->x / GRID_CELL_SIZE);
            int gridY = (int)(p->y / GRID_CELL_SIZE);
            if (gridX >= 0 && gridX < GRID_CELL_SIZE && gridY >= 0 && gridY < GRID_CELL_SIZE) {
                GridCell* cell = &system->grid[gridX][gridY];
                if (cell->count < MAX_PARTICLES / GRID_CELL_SIZE) {
                    cell->particles[cell->count++] = *p;
                }
            }
        }
#endif
    }
}

void ParticleSystem_Render(ParticleSystem* system, SDL_Renderer* renderer, int scale) {
    // Set particle color once (white with alpha based on lifetime)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < GRID_CELL_SIZE; i++) {
        for (int j = 0; j < GRID_CELL_SIZE; j++) {
            GridCell* cell = &system->grid[i][j];
            for (int k = 0; k < cell->count; k++) {
                Particle* p = &cell->particles[k];

                // Calculate distance from the camera (simplified to 2D for now)
                float distance = sqrt(p->x * p->x + p->y * p->y);

                // Skip particles that are far away (LOD)
                if (distance > 200) continue;

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
    }
}