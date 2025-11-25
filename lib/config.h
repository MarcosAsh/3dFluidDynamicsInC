#ifndef CONFIG_H
#define CONFIG_H

#ifndef IX3D
#define IX3D(x, y, z, sizeX, sizeY) ((x) + (y) * (sizeX) + (z) * (sizeX) * (sizeY))
#endif

#ifndef WIDTH
#define WIDTH 1920
#endif

#ifndef HEIGHT
#define HEIGHT 1080
#endif

// Global constants
#define DEPTH 500
#define SCALE 5

// Particle count - used for both CPU and GPU particle systems
// Note: Large values (50000+) require heap allocation, not stack!
#ifndef MAX_PARTICLES
#define MAX_PARTICLES 10000
#endif

#endif // CONFIG_H