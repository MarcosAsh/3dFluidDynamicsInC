#include "./lib/fluid_cube.h"
#include "./lib/coloring.h"
#include <time.h>

const int WIDTH = 500;
const int HEIGHT = 500;
const int DEPTH = 500;

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
                              SDL_WINDOW_SHOWN);

    if (window == NULL) {
        SDL_ExitWithError("Could not create window");
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        SDL_ExitWithError("Could not create renderer");
    }

    printf("SDL window and renderer initialized.\n");

    int running = 1;
    int holdingClick = 0;
    int mode = 0;
    int velThreshold = 5;
    int delayCalculations = 50;
    int lastCalculated = 0;
    int lastSprayed = 0;
    int SCALE = 5;

    srand(time(NULL)); // Seed the random number generator

    FluidCube *fluid = FluidCubeCreate(WIDTH / SCALE, HEIGHT / SCALE, DEPTH / SCALE, 0.001f, 0.0f, 0.001f);
    if (fluid == NULL) {
        SDL_ExitWithError("Could not create fluid cube");
    }

    printf("Fluid cube created.\n");

    while (running) {
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
                        printf("Switched to mode %d\n", mode);
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

        if (SDL_GetTicks() - lastSprayed > 20) {
            lastSprayed = SDL_GetTicks();

            int x = rand() % fluid->sizeX;
            int y = rand() % fluid->sizeY;
            int z = rand() % fluid->sizeZ;

            float density = 100.0f;
            float velX = (rand() % 100) - 50;
            float velY = (rand() % 100) - 50;
            float velZ = (rand() % 100) - 50;

            FluidCubeAddDensity(fluid, x, y, z, density);
            FluidCubeAddVelocity(fluid, x, y, z, velX, velY, velZ);

            printf("Added density and velocity at (%d, %d, %d)\n", x, y, z);
        }

        if (SDL_GetTicks() - lastCalculated > delayCalculations) {
            lastCalculated = SDL_GetTicks();
            FluidCubeStep(fluid);
            printf("Simulation stepped.\n");
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

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

        SDL_RenderPresent(renderer);
    }

    FluidCubeFree(fluid);
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();

    printf("Program exited successfully.\n");
    return 0;
}