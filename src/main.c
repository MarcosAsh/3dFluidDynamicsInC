#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cglm/cglm.h> // Add GLM for matrix calculations

#include "../src/fluid_cube.h" // Update the include path
#include "../lib/coloring.h"
#include "../lib/particle_system.h"
#include "../obj-file-loader/lib/model_loader.h"
#include "../lib/render_model.h"
#include "../lib/opengl_utils.h"

// Window dimensions
#ifndef WIDTH
#define WIDTH 1920
#endif

#ifndef HEIGHT
#define HEIGHT 1080
#endif

// Slider variables
int sliderX = 100;
int sliderY = 50;
int sliderWidth = 200;
int sliderHeight = 20;
int handleWidth = 10;
int handleX;
int isDragging = 0;
float windSpeed = 0.0f;

// Function to render the slider
void renderSlider(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_Rect sliderRect = {sliderX, sliderY, sliderWidth, sliderHeight};
    SDL_RenderFillRect(renderer, &sliderRect);

    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_Rect handleRect = {handleX, sliderY, handleWidth, sliderHeight};
    SDL_RenderFillRect(renderer, &handleRect);
}

// Function to render text
void renderText(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y) {
    SDL_Color color = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create SDL window with OpenGL context
    SDL_Window* window = SDL_CreateWindow("3D Fluid Simulation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
    if (!window) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create OpenGL context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        printf("SDL_GL_CreateContext Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        printf("Failed to initialize GLEW\n");
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Verify OpenGL context (Fix 1)
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("Renderer: %s\n", glGetString(GL_RENDERER));

    // Enable vsync
    SDL_GL_SetSwapInterval(1);

    // Enable depth testing (Fix 5)
    glEnable(GL_DEPTH_TEST);

    // Set up projection and camera using GLM
    mat4 projection, view;
    glm_perspective(glm_rad(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f, projection);
    glm_lookat((vec3){0.0f, 0.0f, 5.0f}, (vec3){0.0f, 0.0f, 0.0f}, (vec3){0.0f, 1.0f, 0.0f}, view);

    // Load shaders
    GLuint particleShaderProgram = createShaderProgram("shaders/particle.vert", "shaders/particle.frag");
    GLuint computeShaderProgram = createComputeShader("shaders/particle.comp");

    // Set projection and view matrices in the shader
    GLuint projectionLoc = glGetUniformLocation(particleShaderProgram, "projection");
    GLuint viewLoc = glGetUniformLocation(particleShaderProgram, "view");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, (const GLfloat*)projection);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, (const GLfloat*)view);

    // Initialize particle system
    ParticleSystem particleSystem;
    ParticleSystem_Init(&particleSystem);

    // Initialize fluid simulation
    Model carModel = loadOBJ("../assests/3d-files/car_model.obj");
    FluidCube* fluidCube = FluidCubeCreate(WIDTH / 5, HEIGHT / 5, 50, 0.001f, 0.0f, 0.001f, &carModel);

    // Create OpenGL buffer for particles
    GLuint particleBuffer;
    glGenBuffers(1, &particleBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_PARTICLES * sizeof(Particle), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffer);

    // Main loop
    int running = 1;
    Uint32 lastTime = SDL_GetTicks();
    while (running) {
        // Clear the screen (Fix 3)
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            // Handle slider input
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                int mouseX = event.button.x;
                int mouseY = event.button.y;
                if (mouseX >= handleX && mouseX <= handleX + handleWidth &&
                    mouseY >= sliderY && mouseY <= sliderY + sliderHeight) {
                    isDragging = 1;
                }
            }
            if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
                isDragging = 0;
            }
            if (event.type == SDL_MOUSEMOTION && isDragging) {
                int mouseX = event.motion.x;
                handleX = mouseX - handleWidth / 2;
                if (handleX < sliderX) handleX = sliderX;
                if (handleX > sliderX + sliderWidth - handleWidth) handleX = sliderX + sliderWidth - handleWidth;
                windSpeed = ((float)(handleX - sliderX) / (sliderWidth - handleWidth)) * 350.0f;
            }
        }

        // Calculate delta time
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Update fluid simulation
        FluidCubeStep(fluidCube);

        // Update particles with compute shader
        glUseProgram(computeShaderProgram);
        glUniform1f(glGetUniformLocation(computeShaderProgram, "dt"), deltaTime);
        glUniform3f(glGetUniformLocation(computeShaderProgram, "wind"), windSpeed, 0.0f, 0.0f);
        glDispatchCompute((MAX_PARTICLES + 255) / 256, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Render particles
        glUseProgram(particleShaderProgram);
        glBindBuffer(GL_ARRAY_BUFFER, particleBuffer);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, x));
        glDrawArrays(GL_POINTS, 0, MAX_PARTICLES);

        // Render the 3D car model
        renderModel(&carModel, SCALE);

        // Swap buffers
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    glDeleteBuffers(1, &particleBuffer);
    glDeleteProgram(particleShaderProgram);
    glDeleteProgram(computeShaderProgram);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}