#define _USE_MATH_DEFINES // Enable M_PI and other math constants
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h> // Include math.h for tanf and M_PI

#include "../lib/fluid_cube.h"
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

// Function to check OpenGL errors
void checkGLError(const char* label) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        printf("OpenGL error at %s: %d\n", label, err);
    }
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

    // Verify OpenGL context
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("Renderer: %s\n", glGetString(GL_RENDERER));

    // Enable vsync
    SDL_GL_SetSwapInterval(1);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Set up projection and camera using manual matrix calculations
    float aspect = (float)WIDTH / HEIGHT;
    float fov = 45.0f;
    float near = 0.1f;
    float far = 100.0f;
    float top = tanf(fov * 0.5f * M_PI / 180.0f) * near;
    float bottom = -top;
    float right = top * aspect;
    float left = -right;

    float projection[16] = {
        (2.0f * near) / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, (2.0f * near) / (top - bottom), 0.0f, 0.0f,
        (right + left) / (right - left), (top + bottom) / (top - bottom), -(far + near) / (far - near), -1.0f,
        0.0f, 0.0f, -(2.0f * far * near) / (far - near), 0.0f
    };

    float eyeX = 0.0f, eyeY = 0.0f, eyeZ = 5.0f;
    float centerX = 0.0f, centerY = 0.0f, centerZ = 0.0f;
    float upX = 0.0f, upY = 1.0f, upZ = 0.0f;

    float forward[3] = {centerX - eyeX, centerY - eyeY, centerZ - eyeZ};
    float forwardLength = sqrtf(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
    forward[0] /= forwardLength;
    forward[1] /= forwardLength;
    forward[2] /= forwardLength;

    float up[3] = {upX, upY, upZ};
    float side[3] = {
        forward[1] * up[2] - forward[2] * up[1],
        forward[2] * up[0] - forward[0] * up[2],
        forward[0] * up[1] - forward[1] * up[0]
    };
    float sideLength = sqrtf(side[0] * side[0] + side[1] * side[1] + side[2] * side[2]);
    side[0] /= sideLength;
    side[1] /= sideLength;
    side[2] /= sideLength;

    up[0] = side[1] * forward[2] - side[2] * forward[1];
    up[1] = side[2] * forward[0] - side[0] * forward[2];
    up[2] = side[0] * forward[1] - side[1] * forward[0];

    float view[16] = {
        side[0], up[0], -forward[0], 0.0f,
        side[1], up[1], -forward[1], 0.0f,
        side[2], up[2], -forward[2], 0.0f,
        -(side[0] * eyeX + side[1] * eyeY + side[2] * eyeZ),
        -(up[0] * eyeX + up[1] * eyeY + up[2] * eyeZ),
        forward[0] * eyeX + forward[1] * eyeY + forward[2] * eyeZ,
        1.0f
    };

    // Load shaders
    GLuint particleShaderProgram = createShaderProgram("../shaders/particle.vert", "../shaders/particle.frag");
    GLuint computeShaderProgram = createComputeShader("../shaders/particle.comp");

    // Set projection and view matrices in the shader
    GLuint projectionLoc = glGetUniformLocation(particleShaderProgram, "projection");
    GLuint viewLoc = glGetUniformLocation(particleShaderProgram, "view");
    if (projectionLoc == -1) {
        printf("Error: 'projection' uniform not found in particle shader.\n");
    }
    if (viewLoc == -1) {
        printf("Error: 'view' uniform not found in particle shader.\n");
    }
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
    checkGLError("After setting projection and view matrices");

    // Initialize particle system
    ParticleSystem particleSystem;
    ParticleSystem_Init(&particleSystem);

    // Initialize fluid simulation
    Model carModel = loadOBJ("../assests/3d-files/car-model.obj");
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
        // Clear the screen
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
        checkGLError("After rendering particles");

        // Render the 3D car model
        renderModel(&carModel, SCALE);
        checkGLError("After rendering model");

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