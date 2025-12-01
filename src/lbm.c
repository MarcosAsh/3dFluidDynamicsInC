#include "../lib/lbm.h"
#include "../lib/opengl_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// D3Q19 weights for initialization
static const float w[19] = {
    1.0f/3.0f,   // rest
    1.0f/18.0f, 1.0f/18.0f, 1.0f/18.0f, 1.0f/18.0f, 1.0f/18.0f, 1.0f/18.0f,
    1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f,
    1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f,
    1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f, 1.0f/36.0f
};

// D3Q19 velocity directions
static const int ex[19] = {0, 1,-1, 0, 0, 0, 0, 1,-1, 1,-1, 1,-1, 1,-1, 0, 0, 0, 0};
static const int ey[19] = {0, 0, 0, 1,-1, 0, 0, 1, 1,-1,-1, 0, 0, 0, 0, 1,-1, 1,-1};
static const int ez[19] = {0, 0, 0, 0, 0, 1,-1, 0, 0, 0, 0, 1, 1,-1,-1, 1, 1,-1,-1};

static float feq(int i, float rho, float ux, float uy, float uz) {
    float eu = ex[i]*ux + ey[i]*uy + ez[i]*uz;
    float u2 = ux*ux + uy*uy + uz*uz;
    float cs2 = 1.0f / 3.0f;
    return w[i] * rho * (1.0f + eu/cs2 + (eu*eu)/(2.0f*cs2*cs2) - u2/(2.0f*cs2));
}

LBMGrid* LBM_Create(int sizeX, int sizeY, int sizeZ, float viscosity) {
    LBMGrid* grid = (LBMGrid*)malloc(sizeof(LBMGrid));
    if (!grid) return NULL;
    
    grid->sizeX = sizeX;
    grid->sizeY = sizeY;
    grid->sizeZ = sizeZ;
    grid->totalCells = sizeX * sizeY * sizeZ;
    
    // tau = 0.5 + 3 * viscosity (lattice units)
    // For air: kinematic viscosity ~ 1.5e-5 m²/s
    // In lattice units with dx=1, dt=1: nu = viscosity
    grid->tau = 0.5f + 3.0f * viscosity;
    if (grid->tau < 0.51f) grid->tau = 0.51f;  // Stability
    if (grid->tau > 2.0f) grid->tau = 2.0f;
    
    printf("LBM Grid: %dx%dx%d (%d cells), tau=%.3f\n", 
           sizeX, sizeY, sizeZ, grid->totalCells, grid->tau);
    
    // Allocate GPU buffers
    size_t fSize = 19 * grid->totalCells * sizeof(float);
    size_t velSize = grid->totalCells * 4 * sizeof(float);
    size_t solidSize = grid->totalCells * sizeof(int);
    
    glGenBuffers(1, &grid->fBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, grid->fBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, fSize, NULL, GL_DYNAMIC_DRAW);
    
    glGenBuffers(1, &grid->fNewBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, grid->fNewBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, fSize, NULL, GL_DYNAMIC_DRAW);
    
    glGenBuffers(1, &grid->velocityBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, grid->velocityBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, velSize, NULL, GL_DYNAMIC_DRAW);
    
    glGenBuffers(1, &grid->solidBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, grid->solidBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, solidSize, NULL, GL_DYNAMIC_DRAW);
    
    // Initialize solid mask to all fluid
    int* solidData = (int*)calloc(grid->totalCells, sizeof(int));
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, solidSize, solidData);
    free(solidData);
    
    // Load shaders
    grid->collideShader = createComputeShader("shaders/lbm_collide.comp");
    grid->streamShader = createComputeShader("shaders/lbm_stream.comp");
    
    if (!grid->collideShader || !grid->streamShader) {
        printf("Failed to create LBM shaders!\n");
        LBM_Free(grid);
        return NULL;
    }
    
    // Get uniform locations
    glUseProgram(grid->collideShader);
    grid->collide_gridSizeLoc = glGetUniformLocation(grid->collideShader, "gridSize");
    grid->collide_tauLoc = glGetUniformLocation(grid->collideShader, "tau");
    grid->collide_inletVelLoc = glGetUniformLocation(grid->collideShader, "inletVelocity");
    
    glUseProgram(grid->streamShader);
    grid->stream_gridSizeLoc = glGetUniformLocation(grid->streamShader, "gridSize");
    
    printf("LBM initialized successfully\n");
    return grid;
}

void LBM_Free(LBMGrid* grid) {
    if (!grid) return;
    
    if (grid->fBuffer) glDeleteBuffers(1, &grid->fBuffer);
    if (grid->fNewBuffer) glDeleteBuffers(1, &grid->fNewBuffer);
    if (grid->velocityBuffer) glDeleteBuffers(1, &grid->velocityBuffer);
    if (grid->solidBuffer) glDeleteBuffers(1, &grid->solidBuffer);
    if (grid->collideShader) glDeleteProgram(grid->collideShader);
    if (grid->streamShader) glDeleteProgram(grid->streamShader);
    
    free(grid);
}

void LBM_SetSolidAABB(LBMGrid* grid, float minX, float minY, float minZ,
                      float maxX, float maxY, float maxZ) {
    // Convert world coords to grid coords
    // Assume grid spans [-4, 4] in x, [-2, 2] in y and z
    float scaleX = grid->sizeX / 8.0f;
    float scaleY = grid->sizeY / 4.0f;
    float scaleZ = grid->sizeZ / 4.0f;
    
    int gMinX = (int)((minX + 4.0f) * scaleX);
    int gMaxX = (int)((maxX + 4.0f) * scaleX);
    int gMinY = (int)((minY + 2.0f) * scaleY);
    int gMaxY = (int)((maxY + 2.0f) * scaleY);
    int gMinZ = (int)((minZ + 2.0f) * scaleZ);
    int gMaxZ = (int)((maxZ + 2.0f) * scaleZ);
    
    // Clamp to grid
    if (gMinX < 0) gMinX = 0;
    if (gMaxX >= grid->sizeX) gMaxX = grid->sizeX - 1;
    if (gMinY < 0) gMinY = 0;
    if (gMaxY >= grid->sizeY) gMaxY = grid->sizeY - 1;
    if (gMinZ < 0) gMinZ = 0;
    if (gMaxZ >= grid->sizeZ) gMaxZ = grid->sizeZ - 1;
    
    printf("LBM solid AABB: grid [%d-%d, %d-%d, %d-%d]\n",
           gMinX, gMaxX, gMinY, gMaxY, gMinZ, gMaxZ);
    
    // Update solid mask
    int* solidData = (int*)calloc(grid->totalCells, sizeof(int));
    
    for (int z = gMinZ; z <= gMaxZ; z++) {
        for (int y = gMinY; y <= gMaxY; y++) {
            for (int x = gMinX; x <= gMaxX; x++) {
                int idx = x + y * grid->sizeX + z * grid->sizeX * grid->sizeY;
                solidData[idx] = 1;
            }
        }
    }
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, grid->solidBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, grid->totalCells * sizeof(int), solidData);
    free(solidData);
}

void LBM_InitializeFlow(LBMGrid* grid, float ux, float uy, float uz) {
    float rho = 1.0f;
    
    // Initialize distribution functions to equilibrium
    float* fData = (float*)malloc(19 * grid->totalCells * sizeof(float));
    float* velData = (float*)malloc(4 * grid->totalCells * sizeof(float));
    
    for (int idx = 0; idx < grid->totalCells; idx++) {
        for (int i = 0; i < 19; i++) {
            fData[idx * 19 + i] = feq(i, rho, ux, uy, uz);
        }
        velData[idx * 4 + 0] = ux;
        velData[idx * 4 + 1] = uy;
        velData[idx * 4 + 2] = uz;
        velData[idx * 4 + 3] = rho;
    }
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, grid->fBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 19 * grid->totalCells * sizeof(float), fData);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, grid->fNewBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 19 * grid->totalCells * sizeof(float), fData);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, grid->velocityBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 4 * grid->totalCells * sizeof(float), velData);
    
    free(fData);
    free(velData);
    
    printf("LBM flow initialized: u=(%.3f, %.3f, %.3f)\n", ux, uy, uz);
}

void LBM_Step(LBMGrid* grid, float inletVelX, float inletVelY, float inletVelZ) {
    // Bind buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, grid->fBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, grid->fNewBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, grid->velocityBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, grid->solidBuffer);
    
    // Collision step
    glUseProgram(grid->collideShader);
    glUniform3i(grid->collide_gridSizeLoc, grid->sizeX, grid->sizeY, grid->sizeZ);
    glUniform1f(grid->collide_tauLoc, grid->tau);
    glUniform3f(grid->collide_inletVelLoc, inletVelX, inletVelY, inletVelZ);
    
    glDispatchCompute((grid->sizeX + 7) / 8, (grid->sizeY + 7) / 8, (grid->sizeZ + 7) / 8);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    
    // Streaming step
    glUseProgram(grid->streamShader);
    glUniform3i(grid->stream_gridSizeLoc, grid->sizeX, grid->sizeY, grid->sizeZ);
    
    glDispatchCompute((grid->sizeX + 7) / 8, (grid->sizeY + 7) / 8, (grid->sizeZ + 7) / 8);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

GLuint LBM_GetVelocityBuffer(LBMGrid* grid) {
    return grid->velocityBuffer;
}

void LBM_ComputeDragForce(LBMGrid* grid, float* forceX, float* forceY, float* forceZ) {
    // Momentum exchange method - would need another compute shader
    // For now, return placeholder
    *forceX = 0.0f;
    *forceY = 0.0f;
    *forceZ = 0.0f;
    
    // TODO: Implement momentum exchange on GPU
    // Sum forces at solid boundaries
}