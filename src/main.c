#define _USE_MATH_DEFINES
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "../lib/fluid_cube.h"
#include "../lib/particle_system.h"
#include "../obj-file-loader/lib/model_loader.h"
#include "../lib/render_model.h"
#include "../lib/opengl_utils.h"

// ADD THESE DEFINITIONS
#ifndef WIDTH
#define WIDTH 1920
#endif

#ifndef HEIGHT
#define HEIGHT 1080
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef MAX_PARTICLES
#define MAX_PARTICLES 50000
#endif

#ifndef SCALE
#define SCALE 5
#endif

// Slider variables
int sliderX = 100;
int sliderY = 50;
int sliderWidth = 200;
int sliderHeight = 20;
int handleWidth = 10;
int handleX = 100;
int isDragging = 0;
float windSpeed = 1.0f;

// Function to check OpenGL errors
void checkGLError(const char* label) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        printf("OpenGL error at %s: 0x%x\n", label, err);
    }
}

int main(int argc, char* argv[]) {
    srand(time(NULL));
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    // Set OpenGL attributes BEFORE creating window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY); // CHANGED TO COMPATIBILITY
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Create SDL window with OpenGL context
    SDL_Window* window = SDL_CreateWindow("3D Fluid Simulation", 
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
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        printf("SDL_GL_CreateContext Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_MakeCurrent(window, glContext);

    // Initialize GLEW
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
    GLuint particleShaderProgram = createShaderProgram("shaders/particle.vert", "shaders/particle.frag");
    if (particleShaderProgram == 0) {
        printf("Failed to create particle shader program\n");
        return 1;
    }
    
    GLuint computeShaderProgram = createComputeShader("shaders/particle.comp");
    if (computeShaderProgram == 0) {
        printf("Failed to create compute shader program\n");
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

    // Initialize particles - WIND TUNNEL STYLE
    Particle particles[MAX_PARTICLES];
    for (int i = 0; i < MAX_PARTICLES; i++) {
        // Particles spawn at the left side
        particles[i].x = -4.0f;
        particles[i].y = ((float)rand() / RAND_MAX - 0.5f) * 3.0f;
        particles[i].z = ((float)rand() / RAND_MAX - 0.5f) * 3.0f;
        
        // Wind tunnel flow: left to right
        particles[i].vx = 0.5f;
        particles[i].vy = ((float)rand() / RAND_MAX - 0.5f) * 0.05f;
        particles[i].vz = ((float)rand() / RAND_MAX - 0.5f) * 0.05f;
        particles[i].life = 1.0f;
    }

    // Create OpenGL buffer for particles
    GLuint particleBuffer;
    glGenBuffers(1, &particleBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_PARTICLES * sizeof(Particle), particles, GL_DYNAMIC_DRAW);
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

    // Initialize particle system (for CPU fallback if needed)
    ParticleSystem particleSystem;
    ParticleSystem_Init(&particleSystem);

    // Initialize fluid simulation
    Model carModel = loadOBJ("/home/marcos_ashton/3dFluidDynamicsInC/assets/3d-files/car-model.obj");    printf("Model loaded: %d vertices, %d faces\n", carModel.vertexCount, carModel.faceCount);
    if (carModel.vertexCount > 0) {
        printf("First vertex: (%.2f, %.2f, %.2f)\n", 
            carModel.vertices[0].x, carModel.vertices[0].y, carModel.vertices[0].z);
    }
    printf("==================\n");
    
    FluidCube* fluidCube = FluidCubeCreate(WIDTH / 5, HEIGHT / 5, 50, 0.001f, 0.0f, 0.001f, &carModel);

    printf("Initialization complete. Starting main loop...\n");

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
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = 0;
            }
        }

        // Calculate delta time
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Update particles with compute shader
        glUseProgram(computeShaderProgram);
        GLint dtLoc = glGetUniformLocation(computeShaderProgram, "dt");
        GLint windLoc = glGetUniformLocation(computeShaderProgram, "wind");
        
        if (dtLoc != -1) glUniform1f(dtLoc, deltaTime);
        if (windLoc != -1) glUniform3f(windLoc, windSpeed * 0.01f, 0.0f, 0.0f);
        
        glDispatchCompute((MAX_PARTICLES + 255) / 256, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        // Render particles
        glUseProgram(particleShaderProgram);
        glBindVertexArray(particleVAO);
        glDrawArrays(GL_POINTS, 0, MAX_PARTICLES);
        checkGLError("After rendering particles");
        
        // Render the 3D car model
        if (carModel.faceCount > 0) {
            glUseProgram(0);  // Switch to fixed pipeline for old OpenGL
            renderModel(&carModel, SCALE);
            checkGLError("After rendering model");
        }

        // Print debug info every 60 frames
        if (frameCount % 60 == 0) {
            printf("Frame %d - FPS: %.1f\n", frameCount, 1.0f / deltaTime);
        }

        // Swap buffers
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
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