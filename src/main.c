#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../lib/fluid_cube.h"
#include "../lib/coloring.h"
#include "../lib/particle_system.h"
#include "../obj-file-loader/lib/model_loader.h"
#include "../lib/render_model.h"


// Slider variables
int sliderX = 100;          // X position of the slider
int sliderY = 50;           // Y position of the slider
int sliderWidth = 200;      // Width of the slider bar
int sliderHeight = 20;      // Height of the slider bar
int handleWidth = 10;       // Width of the slider handle
int handleX = sliderX;      // X position of the slider handle
int isDragging = 0;         // Whether the handle is being dragged
float windSpeed = 0.0f;     // Wind speed in kph (0 to 350)

const int WIDTH = 500;
const int HEIGHT = 500;
const int DEPTH = 500;

// Function to render the slider
void renderSlider(SDL_Renderer* renderer) {
    // Draw the slider bar
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255); // Gray color
    SDL_Rect sliderRect = {sliderX, sliderY, sliderWidth, sliderHeight};
    SDL_RenderFillRect(renderer, &sliderRect);

    // Draw the slider handle
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Light gray color
    SDL_Rect handleRect = {handleX, sliderY, handleWidth, sliderHeight};
    SDL_RenderFillRect(renderer, &handleRect);
}

// Function to render text
void renderText(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y) {
    SDL_Color color = {255, 255, 255, 255}; // White color
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        SDL_ExitWithError("Couldn't init SDL2");
    }

    if (TTF_Init() != 0) {
        SDL_ExitWithError("Couldn't init SDL_ttf");
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

    // Load a font for rendering text
    TTF_Font* font = TTF_OpenFont("/home/marcos-ashton/CLionProjects/3d-fluid-simulation-car/fonts/Arial.ttf", 24); // Replace with your font path
    if (!font) {
        printf("Error: Could not load font\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    printf("SDL window and renderer initialized.\n");

    // Load the 3D model (e.g., a car)
    Model carModel = loadOBJ("car_model.obj");
    if (carModel.vertexCount == 0) {
        printf("Failed to load car model.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    int running = 1;
    int holdingClick = 0;
    int mode = 0;
    int velThreshold = 5;
    int delayCalculations = 50;
    int lastCalculated = 0;
    int lastSprayed = 0;
    int SCALE = 5;

    srand(time(NULL)); // Seed the random number generator

    // Create the fluid cube and pass the car model
    FluidCube *fluid = FluidCubeCreate(WIDTH / SCALE, HEIGHT / SCALE, DEPTH / SCALE, 0.001f, 0.0f, 0.001f, &carModel);
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

                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        // Check if the mouse is over the slider handle
                        int mouseX = event.button.x;
                        int mouseY = event.button.y;
                        if (mouseX >= handleX && mouseX <= handleX + handleWidth &&
                            mouseY >= sliderY && mouseY <= sliderY + sliderHeight) {
                            isDragging = 1; // Start dragging
                        }
                    }
                    break;

                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        isDragging = 0; // Stop dragging
                    }
                    break;

                case SDL_MOUSEMOTION:
                    if (isDragging) {
                        // Update the handle position based on mouse movement
                        int mouseX = event.motion.x;
                        handleX = mouseX - handleWidth / 2;

                        // Clamp the handle position to the slider bounds
                        if (handleX < sliderX) handleX = sliderX;
                        if (handleX > sliderX + sliderWidth - handleWidth) handleX = sliderX + sliderWidth - handleWidth;

                        // Map handle position to wind speed (0 to 350 kph)
                        windSpeed = ((float)(handleX - sliderX) / (sliderWidth - handleWidth)) * 350.0f;
                        printf("Wind speed: %.2f kph\n", windSpeed);
                    }
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

            // Adjust velocity based on wind speed
            velX += windSpeed / 350.0f * 10.0f; // Example: Increase X velocity based on wind speed

            FluidCubeAddDensity(fluid, x, y, z, density);
            FluidCubeAddVelocity(fluid, x, y, z, velX, velY, velZ);

            printf("Added density and velocity at (%d, %d, %d)\n", x, y, z);
        }

        if (SDL_GetTicks() - lastCalculated > delayCalculations) {
            lastCalculated = SDL_GetTicks();
            FluidCubeStep(fluid);
            printf("Simulation stepped.\n");
        }

        // Clear the screen
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

        // Render the slider
        renderSlider(renderer);

        // Render the wind speed text
        char windSpeedText[32];
        sprintf(windSpeedText, "Wind Speed: %.2f kph", windSpeed);
        renderText(renderer, font, windSpeedText, sliderX, sliderY + sliderHeight + 10);

        // Render the 3D car model
        renderModel(renderer, &carModel, SCALE);

        // Present the rendered frame
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    TTF_CloseFont(font);
    freeModel(&carModel);
    FluidCubeFree(fluid);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    printf("Program exited successfully.\n");
    return 0;
}