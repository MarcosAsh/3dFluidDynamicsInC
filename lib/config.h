#ifndef CONFIG_H
#define CONFIG_H

// Define the IX3D macro for 3D array indexing
#define IX3D(x, y, z, sizeX, sizeY) ((x) + (y) * (sizeX) + (z) * (sizeX) * (sizeY))

// Global constants
#define WIDTH 500
#define HEIGHT 500
#define DEPTH 500
#define SCALE 5
#define MAX_PARTICLES 10000

#endif // CONFIG_H