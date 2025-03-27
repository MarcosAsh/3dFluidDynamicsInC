#ifndef CONFIG_H
#define CONFIG_H

#ifndef IX3D
#define IX3D(x, y, z, sizeX, sizeY) ((x) + (y) * (sizeX) + (z) * (sizeX) * (sizeY))
#endif

#ifndef WIDTH
#define WIDTH 500
#endif

#ifndef HEIGHT
#define HEIGHT 500
#endif

// Global constants
#define DEPTH 500
#define SCALE 5
#define MAX_PARTICLES 10000

#endif // CONFIG_H