#include "./lib/fluid_cube.h"
#include "./lib/coloring.h" // Include the coloring functions

const int WIDTH = 500;
const int HEIGHT = 500;
const int DEPTH = 500;

int min(int a, int b) {
    return (a > b) ? b : a;
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        SDL_ExitWithError("Couldn't init SDL2");
    }

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    window = SDL_CreateWindow("3D Fluid Simulation (Visualization : density)",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              WIDTH, HEIGHT,
                              SDL_WINDOW_ALLOW_HIGHDPI);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

    if (window == NULL) {
        SDL_ExitWithError("Could not create window");
    }
    if (renderer == NULL) {
        SDL_ExitWithError("Could not create renderer");
    }

    int running = 1;
    int holdingClick = 0;
    int mode = 0;
    int velThreshold = 5;
    int delayCalculations = 50;
    int lastCalculated = 0;
    int lastSprayed = 0;
    int mx, my;
    Uint32 buttons;

    int SCALE = 5;
    FluidCube *fluid = FluidCubeCreate(WIDTH / SCALE, HEIGHT / SCALE, DEPTH / SCALE, 0.001f, 0.0f, 0.001f);

    int angle = 0;

    while (running) {
    // Handle events (e.g., mouse clicks, key presses)
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                running = 0;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_s) {
                    mode = (mode + 1) % 3;
                    switch (mode) {
                        case 0:
                            SDL_SetWindowTitle(window, "3D Fluid Simulation (Visualization : density)");
                            break;
                        case 1:
                            SDL_SetWindowTitle(window, "3D Fluid Simulation (Visualization : velocity)");
                            break;
                        case 2:
                            SDL_SetWindowTitle(window, "3D Fluid Simulation (Visualization : density and velocity)");
                            break;
                    }
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    holdingClick = 1;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    holdingClick = 0;
                }
                break;
        }
    }

    // Add random density and velocity to the fluid
    if (SDL_GetTicks() - lastSprayed > 20) { // Add density/velocity every 20ms
        lastSprayed = SDL_GetTicks();

        // Random position in the fluid grid
        int x = rand() % fluid->sizeX;
        int y = rand() % fluid->sizeY;
        int z = rand() % fluid->sizeZ;

        // Random density and velocity
        float density = 100.0f; // Fixed density value
        float velX = (rand() % 100) - 50; // Random velocity in X direction (-50 to 50)
        float velY = (rand() % 100) - 50; // Random velocity in Y direction (-50 to 50)
        float velZ = (rand() % 100) - 50; // Random velocity in Z direction (-50 to 50)

        // Add density and velocity to the fluid
        FluidCubeAddDensity(fluid, x, y, z, density);
        FluidCubeAddVelocity(fluid, x, y, z, velX, velY, velZ);
    }

    // Step the simulation
    if (SDL_GetTicks() - lastCalculated > delayCalculations) {
        lastCalculated = SDL_GetTicks();
        FluidCubeStep(fluid);
    }

    // Fill background with black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Render the fluid simulation
    for (int i = 0; i < fluid->sizeX; i++) {
        for (int j = 0; j < fluid->sizeY; j++) {
            for (int k = 0; k < fluid->sizeZ; k++) {
                SDL_Rect rect;
                rect.x = i * SCALE;
                rect.y = j * SCALE;
                rect.w = SCALE;
                rect.h = SCALE;

                Uint8 r, g, b;
                float density = fluid->density[IX3D(i, j, k, fluid->sizeX, fluid->sizeY)];
                float velX = fluid->Vx[IX3D(i, j, k, fluid->sizeX, fluid->sizeY)];
                float velY = fluid->Vy[IX3D(i, j, k, fluid->sizeX, fluid->sizeY)];
                float velZ = fluid->Vz[IX3D(i, j, k, fluid->sizeX, fluid->sizeY)];

                // Use coloring functions to set the color based on the mode
                if (mode == 0) {
                    DensityToColor(density, &r, &g, &b);
                } else if (mode == 1) {
                    VelocityToColor(velX, velY, velZ, &r, &g, &b, velThreshold);
                } else if (mode == 2) {
                    DensityAndVelocityToColor(density, velX, velY, velZ, &r, &g, &b, velThreshold);
                }

                SDL_SetRenderDrawColor(renderer, r, g, b, 255);
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }

    // Show the current frame
    SDL_RenderPresent(renderer);
}

    // Clean up
    FluidCubeFree(fluid);
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}