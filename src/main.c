#define _USE_MATH_DEFINES
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <float.h>

#include "../lib/fluid_cube.h"
#include "../lib/particle_system.h"
#include "../obj-file-loader/lib/model_loader.h"
#include "../lib/render_model.h"
#include "../lib/opengl_utils.h"

#ifndef WIDTH
#define WIDTH 1920
#endif

#ifndef HEIGHT
#define HEIGHT 1080
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef SCALE
#define SCALE 5
#endif

// Use MAX_PARTICLES from config.h, but define a local one for GPU if needed
#define GPU_PARTICLES MAX_PARTICLES

// Slider variables
int sliderX = 100;
int sliderY = 50;
int sliderWidth = 200;
int sliderHeight = 20;
int handleWidth = 10;
int handleX = 100;
int isDragging = 0;
float windSpeed = 1.0f;

// Car bounding box structure
typedef struct {
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
    float centerX, centerY, centerZ;
} CarBounds;

// Function to compute bounding box from model
CarBounds computeModelBounds(Model* model, float scale, float offsetX, float offsetY, float offsetZ) {
    CarBounds bounds;
    
    if (model == NULL || model->vertexCount == 0) {
        // Default bounds if no model
        printf("Warning: No model vertices, using default bounds\n");
        bounds.minX = bounds.minY = bounds.minZ = -0.5f;
        bounds.maxX = bounds.maxY = bounds.maxZ = 0.5f;
        bounds.centerX = bounds.centerY = bounds.centerZ = 0.0f;
        return bounds;
    }
    
    bounds.minX = bounds.minY = bounds.minZ = FLT_MAX;
    bounds.maxX = bounds.maxY = bounds.maxZ = -FLT_MAX;
    
    for (int i = 0; i < model->vertexCount; i++) {
        float x = model->vertices[i].x * scale + offsetX;
        float y = model->vertices[i].y * scale + offsetY;
        float z = model->vertices[i].z * scale + offsetZ;
        
        if (x < bounds.minX) bounds.minX = x;
        if (y < bounds.minY) bounds.minY = y;
        if (z < bounds.minZ) bounds.minZ = z;
        if (x > bounds.maxX) bounds.maxX = x;
        if (y > bounds.maxY) bounds.maxY = y;
        if (z > bounds.maxZ) bounds.maxZ = z;
    }
    
    bounds.centerX = (bounds.minX + bounds.maxX) * 0.5f;
    bounds.centerY = (bounds.minY + bounds.maxY) * 0.5f;
    bounds.centerZ = (bounds.minZ + bounds.maxZ) * 0.5f;
    
    printf("Model bounds: min(%.2f, %.2f, %.2f) max(%.2f, %.2f, %.2f)\n",
           bounds.minX, bounds.minY, bounds.minZ,
           bounds.maxX, bounds.maxY, bounds.maxZ);
    printf("Model center: (%.2f, %.2f, %.2f)\n",
           bounds.centerX, bounds.centerY, bounds.centerZ);
    
    return bounds;
}

// Function to check OpenGL errors
void checkGLError(const char* label) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        printf("OpenGL error at %s: 0x%x\n", label, err);
    }
}

int main(int argc, char* argv[]) {
    printf("Starting 3D Fluid Simulation...\n");
    srand(time(NULL));
    
    // Initialize SDL
    printf("Initializing SDL...\n");
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    // Set OpenGL attributes BEFORE creating window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Create SDL window with OpenGL context
    printf("Creating window...\n");
    SDL_Window* window = SDL_CreateWindow("3D Fluid Simulation - Wind Tunnel with Collisions", 
                                        SDL_WINDOWPOS_CENTERED, 
                                        SDL_WINDOWPOS_CENTERED, 
                                        WIDTH, HEIGHT, 
                                        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create OpenGL context
    printf("Creating OpenGL context...\n");
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        printf("SDL_GL_CreateContext Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_MakeCurrent(window, glContext);

    // Initialize GLEW
    printf("Initializing GLEW...\n");
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        printf("Failed to initialize GLEW\n");
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    glGetError(); // Clear GLEW errors

    // Verify OpenGL context
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("Renderer: %s\n", glGetString(GL_RENDERER));

    // Enable vsync
    SDL_GL_SetSwapInterval(1);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    checkGLError("After initial GL setup");

    // Set up projection matrix
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

    // Set up view matrix - angled view for better 3D perception
    float eyeX = 3.0f, eyeY = 2.0f, eyeZ = 5.0f;
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
    printf("Loading shaders...\n");
    GLuint particleShaderProgram = createShaderProgram("shaders/particle.vert", "shaders/particle.frag");
    if (particleShaderProgram == 0) {
        printf("Failed to create particle shader program\n");
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    GLuint computeShaderProgram = createComputeShader("shaders/particle.comp");
    if (computeShaderProgram == 0) {
        printf("Failed to create compute shader program\n");
        glDeleteProgram(particleShaderProgram);
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    checkGLError("After shader creation");

    // Set projection and view matrices in the shader
    glUseProgram(particleShaderProgram);
    GLuint projectionLoc = glGetUniformLocation(particleShaderProgram, "projection");
    GLuint viewLoc = glGetUniformLocation(particleShaderProgram, "view");
    
    if (projectionLoc != -1) {
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection);
    }
    if (viewLoc != -1) {
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
    }
    
    checkGLError("After setting uniforms");

    // Load car model first to compute bounds
    printf("Loading car model...\n");
    Model carModel = loadOBJ("assets/3d-files/car-model.obj");
    
    // Check if model loaded - try alternative paths if not
    if (carModel.vertexCount == 0) {
        printf("Trying alternative path...\n");
        carModel = loadOBJ("/home/marcos_ashton/3dFluidDynamicsInC/assets/3d-files/car-model.obj");
    }
    if (carModel.vertexCount == 0) {
        printf("Trying another path...\n");
        carModel = loadOBJ("../assets/3d-files/car-model.obj");
    }
    
    printf("Model loaded: %d vertices, %d faces\n", carModel.vertexCount, carModel.faceCount);
    
    // Compute car bounds using the same scale/offset as render_model.c
    float modelScale = 0.05f;
    float offsetX = 0.0f;
    float offsetY = -0.2f;
    float offsetZ = -0.9f;
    
    CarBounds carBounds = computeModelBounds(&carModel, modelScale, offsetX, offsetY, offsetZ);
    
    // Get uniform locations for collision parameters in compute shader
    glUseProgram(computeShaderProgram);
    GLint carMinLoc = glGetUniformLocation(computeShaderProgram, "carMin");
    GLint carMaxLoc = glGetUniformLocation(computeShaderProgram, "carMax");
    GLint carCenterLoc = glGetUniformLocation(computeShaderProgram, "carCenter");
    GLint collisionEnabledLoc = glGetUniformLocation(computeShaderProgram, "collisionEnabled");
    
    printf("Collision uniform locations: carMin=%d, carMax=%d, carCenter=%d, enabled=%d\n",
           carMinLoc, carMaxLoc, carCenterLoc, collisionEnabledLoc);

    // IMPORTANT: Allocate particles on HEAP, not stack (would cause stack overflow!)
    printf("Allocating %d particles on heap...\n", GPU_PARTICLES);
    Particle* particles = (Particle*)malloc(GPU_PARTICLES * sizeof(Particle));
    if (!particles) {
        printf("Failed to allocate particle memory!\n");
        freeModel(&carModel);
        glDeleteProgram(particleShaderProgram);
        glDeleteProgram(computeShaderProgram);
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Initialize particles - WIND TUNNEL STYLE
    for (int i = 0; i < GPU_PARTICLES; i++) {
        // Particles spawn at the left side
        particles[i].x = -4.0f + ((float)rand() / RAND_MAX) * 0.5f;
        particles[i].y = ((float)rand() / RAND_MAX - 0.5f) * 3.0f;
        particles[i].z = ((float)rand() / RAND_MAX - 0.5f) * 3.0f;
        particles[i].padding1 = 0.0f;
        
        // Wind tunnel flow: left to right
        particles[i].vx = 0.5f + ((float)rand() / RAND_MAX) * 0.2f;
        particles[i].vy = ((float)rand() / RAND_MAX - 0.5f) * 0.05f;
        particles[i].vz = ((float)rand() / RAND_MAX - 0.5f) * 0.05f;
        particles[i].life = 1.0f;
    }

    // Create OpenGL buffer for particles
    printf("Creating particle buffer...\n");
    GLuint particleBuffer;
    glGenBuffers(1, &particleBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, GPU_PARTICLES * sizeof(Particle), particles, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffer);
    checkGLError("After creating particle buffer");

    // Create VAO for particles
    GLuint particleVAO;
    glGenVertexArrays(1, &particleVAO);
    glBindVertexArray(particleVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, particleBuffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)0);
    
    glBindVertexArray(0);
    checkGLError("After creating VAO");

    // Initialize fluid simulation (optional - can be NULL if not using fluid sim)
    printf("Creating fluid cube...\n");
    FluidCube* fluidCube = NULL;
    if (carModel.vertexCount > 0) {
        fluidCube = FluidCubeCreate(WIDTH / 10, HEIGHT / 10, 20, 0.001f, 0.0f, 0.001f, &carModel);
    }

    printf("Initialization complete. Starting main loop...\n");
    printf("Particle collisions with car model ENABLED\n");
    printf("Controls: C = toggle collisions, UP/DOWN = adjust wind speed, ESC = quit\n");

    // Collision toggle
    int collisionEnabled = 1;

    // Main loop
    int running = 1;
    Uint32 lastTime = SDL_GetTicks();
    int frameCount = 0;
    
    while (running) {
        frameCount++;
        
        // Clear the screen
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f); // Dark blue background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = 0;
                } else if (event.key.keysym.sym == SDLK_c) {
                    // Toggle collision
                    collisionEnabled = !collisionEnabled;
                    printf("Collisions %s\n", collisionEnabled ? "ENABLED" : "DISABLED");
                } else if (event.key.keysym.sym == SDLK_UP) {
                    windSpeed += 0.5f;
                    printf("Wind speed: %.1f\n", windSpeed);
                } else if (event.key.keysym.sym == SDLK_DOWN) {
                    windSpeed -= 0.5f;
                    if (windSpeed < 0.0f) windSpeed = 0.0f;
                    printf("Wind speed: %.1f\n", windSpeed);
                }
            }
        }

        // Calculate delta time
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Update particles with compute shader
        glUseProgram(computeShaderProgram);
        
        // Set time and wind uniforms
        GLint dtLoc = glGetUniformLocation(computeShaderProgram, "dt");
        GLint windLoc = glGetUniformLocation(computeShaderProgram, "wind");
        
        if (dtLoc != -1) glUniform1f(dtLoc, deltaTime);
        if (windLoc != -1) glUniform3f(windLoc, windSpeed * 0.01f, 0.0f, 0.0f);
        
        // Set collision parameters
        if (carMinLoc != -1) {
            glUniform3f(carMinLoc, carBounds.minX, carBounds.minY, carBounds.minZ);
        }
        if (carMaxLoc != -1) {
            glUniform3f(carMaxLoc, carBounds.maxX, carBounds.maxY, carBounds.maxZ);
        }
        if (carCenterLoc != -1) {
            glUniform3f(carCenterLoc, carBounds.centerX, carBounds.centerY, carBounds.centerZ);
        }
        if (collisionEnabledLoc != -1) {
            glUniform1i(collisionEnabledLoc, collisionEnabled);
        }
        
        glDispatchCompute((GPU_PARTICLES + 255) / 256, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        // Render particles
        glUseProgram(particleShaderProgram);
        glBindVertexArray(particleVAO);
        glDrawArrays(GL_POINTS, 0, GPU_PARTICLES);
        checkGLError("After rendering particles");
        
        // Render the 3D car model
        if (carModel.faceCount > 0) {
            glUseProgram(0);  // Switch to fixed pipeline for old OpenGL
            renderModel(&carModel, SCALE);
            checkGLError("After rendering model");
        }

        // Print debug info every 60 frames
        if (frameCount % 60 == 0) {
            printf("Frame %d - FPS: %.1f - Collisions: %s - Wind: %.1f\n", 
                   frameCount, 1.0f / deltaTime,
                   collisionEnabled ? "ON" : "OFF",
                   windSpeed);
        }

        // Swap buffers
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    printf("Cleaning up...\n");
    free(particles);  // Free heap-allocated particles
    freeModel(&carModel);
    if (fluidCube) FluidCubeFree(fluidCube);
    glDeleteVertexArrays(1, &particleVAO);
    glDeleteBuffers(1, &particleBuffer);
    glDeleteProgram(particleShaderProgram);
    glDeleteProgram(computeShaderProgram);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    printf("Cleanup complete. Exiting.\n");
    return 0;
}