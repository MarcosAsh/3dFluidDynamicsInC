#ifndef LBM_H
#define LBM_H

#include <GL/glew.h>

// LBM grid configuration
typedef struct {
    int sizeX, sizeY, sizeZ;
    int totalCells;
    
    // Relaxation parameter (related to viscosity)
    // tau = 0.5 + viscosity / (cs^2 * dt)
    // For stability: tau > 0.5, typically 0.6 - 2.0
    float tau;
    
    // GPU buffers
    GLuint fBuffer;         // Distribution functions (19 per cell)
    GLuint fNewBuffer;      // Double buffer for streaming
    GLuint velocityBuffer;  // Velocity field for particles (vec4: xyz=vel, w=rho)
    GLuint solidBuffer;     // Solid mask (1=solid, 0=fluid)
    
    // Shaders
    GLuint collideShader;
    GLuint streamShader;
    
    // Uniform locations
    GLint collide_gridSizeLoc;
    GLint collide_tauLoc;
    GLint collide_inletVelLoc;
    GLint stream_gridSizeLoc;
} LBMGrid;

// Initialize LBM grid
LBMGrid* LBM_Create(int sizeX, int sizeY, int sizeZ, float viscosity);

// Free LBM resources
void LBM_Free(LBMGrid* grid);

// Set solid cells from car model bounds
void LBM_SetSolidAABB(LBMGrid* grid, float minX, float minY, float minZ,
                       float maxX, float maxY, float maxZ);

// Set solid cells from triangle mesh (more accurate)
void LBM_SetSolidFromMesh(LBMGrid* grid, float* triangles, int numTriangles);

// Initialize with uniform velocity
void LBM_InitializeFlow(LBMGrid* grid, float ux, float uy, float uz);

// Run one LBM step (collision + streaming)
void LBM_Step(LBMGrid* grid, float inletVelX, float inletVelY, float inletVelZ);

// Get velocity buffer for particle shader to sample
GLuint LBM_GetVelocityBuffer(LBMGrid* grid);

// Compute drag force on solid (momentum exchange)
void LBM_ComputeDragForce(LBMGrid* grid, float* forceX, float* forceY, float* forceZ);

#endif // LBM_H