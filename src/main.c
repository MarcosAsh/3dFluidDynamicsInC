#define _USE_MATH_DEFINES
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

#include "../lib/fluid_cube.h"
#include "../lib/coloring.h"
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

#ifndef MAX_PARTICLES
#define MAX_PARTICLES 50000
#endif

float windSpeed = 0.0f;
int M_PI = 3.14159265358979323846;

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
        printf("OpenGL error at %s: 0x%x\n", label, err);
    }
}

void makeIdentity(float* mat) {
    memset(mat, 0, 16 * sizeof(float));
    mat[0] = mat[5] = mat[10] = mat[15] = 1.0f;
}

void makePerspective(float* mat, float fovy, float aspect, float near, float far) {
    float f = 1.0f / tanf(fovy * M_PI / 360.0f);
    memset(mat, 0, 16 * sizeof(float));
    
    mat[0] = f / aspect;
    mat[5] = f;
    mat[10] = (far + near) / (near - far);
    mat[11] = -1.0f;
    mat[14] = (2.0f * far * near) / (near - far);
}

void makeLookAt(float* mat, float eyeX, float eyeY, float eyeZ,
                float centerX, float centerY, float centerZ,
                float upX, float upY, float upZ) {
    float fx = centerX - eyeX;
    float fy = centerY - eyeY;
    float fz = centerZ - eyeZ;
    float flen = sqrtf(fx*fx + fy*fy + fz*fz);
    fx /= flen; fy /= flen; fz /= flen;
    
    float sx = fy * upZ - fz * upY;
    float sy = fz * upX - fx * upZ;
    float sz = fx * upY - fy * upX;
    float slen = sqrtf(sx*sx + sy*sy + sz*sz);
    sx /= slen; sy /= slen; sz /= slen;
    
    float ux = sy * fz - sz * fy;
    float uy = sz * fx - sx * fz;
    float uz = sx * fy - sy * fx;
    
    memset(mat, 0, 16 * sizeof(float));
    mat[0] = sx;   mat[4] = ux;   mat[8]  = -fx;  mat[12] = 0.0f;
    mat[1] = sy;   mat[5] = uy;   mat[9]  = -fy;  mat[13] = 0.0f;
    mat[2] = sz;   mat[6] = uz;   mat[10] = -fz;  mat[14] = 0.0f;
    mat[3] = 0.0f; mat[7] = 0.0f; mat[11] = 0.0f; mat[15] = 1.0f;
    
    mat[12] = -(sx * eyeX + sy * eyeY + sz * eyeZ);
    mat[13] = -(ux * eyeX + uy * eyeY + uz * eyeZ);
    mat[14] = (fx * eyeX + fy * eyeY + fz * eyeZ);
}

int main(int argc, char* argv[]) {
    srand(time(NULL));
    
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

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

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        printf("SDL_GL_CreateContext Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_MakeCurrent(window, glContext);

    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        printf("Failed to initialize GLEW\n");
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    glGetError(); // Clear GLEW errors

    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("Renderer: %s\n", glGetString(GL_RENDERER));

    SDL_GL_SetSwapInterval(1);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    
    checkGLError("After initial GL setup");

    float projection[16];
    makePerspective(projection, 45.0f, (float)WIDTH / HEIGHT, 0.1f, 100.0f);

    float view[16];
    // Camera positioned at an angle to see depth
    makeLookAt(view, 3.0f, 2.0f, 5.0f,  // Eye position (angle view)
                    0.0f, 0.0f, 0.0f,   // Look at center
                    0.0f, 1.0f, 0.0f);  // Up direction
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

    glUseProgram(particleShaderProgram);
    GLuint projectionLoc = glGetUniformLocation(particleShaderProgram, "projection");
    GLuint viewLoc = glGetUniformLocation(particleShaderProgram, "view");
    
    if (projectionLoc != -1) glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection);
    if (viewLoc != -1) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
    
    checkGLError("After setting uniforms");

    // Initialize particles
    Particle particles[MAX_PARTICLES];
    for (int i = 0; i < MAX_PARTICLES; i++) {
        // Particles spawn at the left side (-4.0) and spread across Y and Z
        particles[i].x = -4.0f;  // All start from left side
        particles[i].y = ((float)rand() / RAND_MAX - 0.5f) * 3.0f;  // Vertical spread
        particles[i].z = ((float)rand() / RAND_MAX - 0.5f) * 3.0f;  // Depth spread
        particles[i].vx = 0.5f; // Main wind direction
        particles[i].vy = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
        particles[i].vz = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
        particles[i].life = 1.0f;
    }

    GLuint particleBuffer;
    glGenBuffers(1, &particleBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_PARTICLES * sizeof(Particle), particles, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffer);
    checkGLError("After creating particle buffer");

    GLuint particleVAO;
    glGenVertexArrays(1, &particleVAO);
    glBindVertexArray(particleVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, particleBuffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)0);
    
    glBindVertexArray(0);
    checkGLError("After creating VAO");

    printf("Initialization complete. Starting main loop...\n");

    int running = 1;
    Uint32 lastTime = SDL_GetTicks();
    int frameCount = 0;
    
    while (running) {
        frameCount++;
        
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = 0;
            }
        }

        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        glUseProgram(computeShaderProgram);
        GLint dtLoc = glGetUniformLocation(computeShaderProgram, "dt");
        GLint windLoc = glGetUniformLocation(computeShaderProgram, "wind");
        
        if (dtLoc != -1) glUniform1f(dtLoc, deltaTime);
        if (windLoc != -1) glUniform3f(windLoc, windSpeed * 0.01f, 0.0f, 0.0f);
        
        glDispatchCompute((MAX_PARTICLES + 255) / 256, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        glUseProgram(particleShaderProgram);
        glBindVertexArray(particleVAO);
        glDrawArrays(GL_POINTS, 0, MAX_PARTICLES);

        if (frameCount % 60 == 0) {
            printf("Frame %d - FPS: %.1f\n", frameCount, 1.0f / deltaTime);
        }

        SDL_GL_SwapWindow(window);
    }

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
