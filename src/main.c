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

// Camera rotation variables
float cameraAngleY = 0.0f;
float cameraAngleX = 0.3f;
float cameraDistance = 6.0f;
float cameraTargetX = 0.0f;
float cameraTargetY = 0.0f;
float cameraTargetZ = 0.0f;

// Mouse control variables
int mouseDown = 0;
int lastMouseX = 0;
int lastMouseY = 0;

// Car bounding box structure
typedef struct {
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
    float centerX, centerY, centerZ;
} CarBounds;

// GPU Triangle struct (must match shader)
typedef struct {
    float v0x, v0y, v0z, pad0;
    float v1x, v1y, v1z, pad1;
    float v2x, v2y, v2z, pad2;
} GPUTriangle;

// Computes model bounds on a togglable definition of the model for better performance
CarBounds computeModelBounds(Model* model, float scale, float offsetX, float offsetY, float offsetZ, float rotationY) {
    CarBounds bounds;
    
    if (model == NULL || model->vertexCount == 0) {
        printf("Warning: No model vertices, using default bounds\n");
        bounds.minX = bounds.minY = bounds.minZ = -0.5f;
        bounds.maxX = bounds.maxY = bounds.maxZ = 0.5f;
        bounds.centerX = bounds.centerY = bounds.centerZ = 0.0f;
        return bounds;
    }
    
    float radY = rotationY * M_PI / 180.0f;
    float cosY = cosf(radY);
    float sinY = sinf(radY);
    
    bounds.minX = bounds.minY = bounds.minZ = FLT_MAX;
    bounds.maxX = bounds.maxY = bounds.maxZ = -FLT_MAX;
    
    // Use vertices directly (not through faces)
    for (int i = 0; i < model->vertexCount; i++) {
        float x = model->vertices[i].x * scale + offsetX;
        float y = model->vertices[i].y * scale + offsetY;
        float z = model->vertices[i].z * scale + offsetZ;
        
        float rotatedX = x * cosY - z * sinY;
        float rotatedZ = x * sinY + z * cosY;
        
        if (rotatedX < bounds.minX) bounds.minX = rotatedX;
        if (y < bounds.minY) bounds.minY = y;
        if (rotatedZ < bounds.minZ) bounds.minZ = rotatedZ;
        if (rotatedX > bounds.maxX) bounds.maxX = rotatedX;
        if (y > bounds.maxY) bounds.maxY = y;
        if (rotatedZ > bounds.maxZ) bounds.maxZ = rotatedZ;
    }
    
    bounds.centerX = (bounds.minX + bounds.maxX) * 0.5f;
    bounds.centerY = (bounds.minY + bounds.maxY) * 0.5f;
    bounds.centerZ = (bounds.minZ + bounds.maxZ) * 0.5f;
    
    printf("Model bounds (rotated %.1f deg): min(%.2f, %.2f, %.2f) max(%.2f, %.2f, %.2f)\n",
           rotationY,
           bounds.minX, bounds.minY, bounds.minZ,
           bounds.maxX, bounds.maxY, bounds.maxZ);
    printf("Model center: (%.2f, %.2f, %.2f)\n",
           bounds.centerX, bounds.centerY, bounds.centerZ);
    
    return bounds;
}

// Create triangle buffer for per-triangle collision
GPUTriangle* createTriangleBuffer(Model* model, float scale, float offsetX, float offsetY, float offsetZ, float rotationY, int* outCount) {
    *outCount = 0;
    
    if (!model || model->faceCount == 0 || model->vertexCount == 0) {
        printf("No model data for triangle buffer\n");
        return NULL;
    }
    
    printf("Creating triangle buffer: %d faces, %d vertices\n", model->faceCount, model->vertexCount);
    
    GPUTriangle* tris = (GPUTriangle*)malloc(model->faceCount * sizeof(GPUTriangle));
    if (!tris) {
        printf("Failed to allocate triangle buffer!\n");
        return NULL;
    }
    
    float radY = rotationY * M_PI / 180.0f;
    float cosY = cosf(radY);
    float sinY = sinf(radY);
    
    int validCount = 0;
    for (int i = 0; i < model->faceCount; i++) {
        int idx0 = model->faces[i].v1 - 1;
        int idx1 = model->faces[i].v2 - 1;
        int idx2 = model->faces[i].v3 - 1;
        
        // Validate indices
        if (idx0 < 0 || idx0 >= model->vertexCount ||
            idx1 < 0 || idx1 >= model->vertexCount ||
            idx2 < 0 || idx2 >= model->vertexCount) {
            printf("Warning: Invalid face %d indices: %d, %d, %d (max: %d)\n", 
                   i, idx0, idx1, idx2, model->vertexCount - 1);
            continue;
        }
        
        Vertex v0 = model->vertices[idx0];
        Vertex v1 = model->vertices[idx1];
        Vertex v2 = model->vertices[idx2];
        
        float x0 = v0.x * scale + offsetX;
        float y0 = v0.y * scale + offsetY;
        float z0 = v0.z * scale + offsetZ;
        tris[validCount].v0x = x0 * cosY - z0 * sinY;
        tris[validCount].v0y = y0;
        tris[validCount].v0z = x0 * sinY + z0 * cosY;
        tris[validCount].pad0 = 0;
        
        float x1 = v1.x * scale + offsetX;
        float y1 = v1.y * scale + offsetY;
        float z1 = v1.z * scale + offsetZ;
        tris[validCount].v1x = x1 * cosY - z1 * sinY;
        tris[validCount].v1y = y1;
        tris[validCount].v1z = x1 * sinY + z1 * cosY;
        tris[validCount].pad1 = 0;
        
        float x2 = v2.x * scale + offsetX;
        float y2 = v2.y * scale + offsetY;
        float z2 = v2.z * scale + offsetZ;
        tris[validCount].v2x = x2 * cosY - z2 * sinY;
        tris[validCount].v2y = y2;
        tris[validCount].v2z = x2 * sinY + z2 * cosY;
        tris[validCount].pad2 = 0;
        
        validCount++;
    }
    
    *outCount = validCount;
    printf("Created %d valid triangles\n", validCount);
    return tris;
}

// Function to check OpenGL errors
void checkGLError(const char* label) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        printf("OpenGL error at %s: 0x%x\n", label, err);
    }
}

// Calculate view matrix from camera angles
void calculateViewMatrix(float* view, float angleY, float angleX, float distance, 
                         float targetX, float targetY, float targetZ) {
    float eyeX = targetX + distance * sinf(angleY) * cosf(angleX);
    float eyeY = targetY + distance * sinf(angleX);
    float eyeZ = targetZ + distance * cosf(angleY) * cosf(angleX);
    
    float forward[3] = {targetX - eyeX, targetY - eyeY, targetZ - eyeZ};
    float forwardLength = sqrtf(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
    forward[0] /= forwardLength;
    forward[1] /= forwardLength;
    forward[2] /= forwardLength;
    
    float up[3] = {0.0f, 1.0f, 0.0f};
    
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
    
    view[0] = side[0];
    view[1] = up[0];
    view[2] = -forward[0];
    view[3] = 0.0f;
    
    view[4] = side[1];
    view[5] = up[1];
    view[6] = -forward[1];
    view[7] = 0.0f;
    
    view[8] = side[2];
    view[9] = up[2];
    view[10] = -forward[2];
    view[11] = 0.0f;
    
    view[12] = -(side[0] * eyeX + side[1] * eyeY + side[2] * eyeZ);
    view[13] = -(up[0] * eyeX + up[1] * eyeY + up[2] * eyeZ);
    view[14] = forward[0] * eyeX + forward[1] * eyeY + forward[2] * eyeZ;
    view[15] = 1.0f;
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

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

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

    printf("Creating OpenGL context...\n");
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        printf("SDL_GL_CreateContext Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_MakeCurrent(window, glContext);

    printf("Initializing GLEW...\n");
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        printf("Failed to initialize GLEW\n");
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    glGetError();

    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("Renderer: %s\n", glGetString(GL_RENDERER));

    SDL_GL_SetSwapInterval(1);

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

    float view[16];
    calculateViewMatrix(view, cameraAngleY, cameraAngleX, cameraDistance,
                        cameraTargetX, cameraTargetY, cameraTargetZ);

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

    glUseProgram(particleShaderProgram);
    GLuint projectionLoc = glGetUniformLocation(particleShaderProgram, "projection");
    GLuint viewLoc = glGetUniformLocation(particleShaderProgram, "view");
    
    if (projectionLoc != -1) {
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection);
    }
    
    checkGLError("After setting uniforms");

    // Load car model
    printf("Loading car model...\n");
    Model carModel = loadOBJ("assets/3d-files/car-model.obj");
    
    if (carModel.vertexCount == 0) {
        printf("Trying alternative path...\n");
        carModel = loadOBJ("/home/marcos_ashton/3dFluidDynamicsInC/assets/3d-files/car-model.obj");
    }
    if (carModel.vertexCount == 0) {
        printf("Trying another path...\n");
        carModel = loadOBJ("../assets/3d-files/car-model.obj");
    }
    
    printf("Model loaded: %d vertices, %d faces\n", carModel.vertexCount, carModel.faceCount);
    
    // Car transform parameters 
    float modelScale = 0.05f;
    float offsetX = 0.0f;
    float offsetY = -0.2f;
    float offsetZ = -0.9f;
    float carRotationY = 90.0f;  // must be the same as render_model.c
    
    CarBounds carBounds = computeModelBounds(&carModel, modelScale, offsetX, offsetY, offsetZ, carRotationY);
    
    // Get uniform locations for compute shader
    glUseProgram(computeShaderProgram);
    GLint carMinLoc = glGetUniformLocation(computeShaderProgram, "carMin");
    GLint carMaxLoc = glGetUniformLocation(computeShaderProgram, "carMax");
    GLint carCenterLoc = glGetUniformLocation(computeShaderProgram, "carCenter");
    GLint collisionModeLoc = glGetUniformLocation(computeShaderProgram, "collisionMode");
    GLint numTrianglesLoc = glGetUniformLocation(computeShaderProgram, "numTriangles");
    
    printf("Collision uniform locations: carMin=%d, carMax=%d, carCenter=%d, mode=%d, numTris=%d\n",
           carMinLoc, carMaxLoc, carCenterLoc, collisionModeLoc, numTrianglesLoc);

    // Allocate particles on heap
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
    
    // Initialize particles
    for (int i = 0; i < GPU_PARTICLES; i++) {
        particles[i].x = -4.0f + ((float)rand() / RAND_MAX) * 0.5f;
        particles[i].y = ((float)rand() / RAND_MAX - 0.5f) * 3.0f;
        particles[i].z = ((float)rand() / RAND_MAX - 0.5f) * 3.0f;
        particles[i].padding1 = 0.0f;
        particles[i].vx = 0.5f + ((float)rand() / RAND_MAX) * 0.2f;
        particles[i].vy = ((float)rand() / RAND_MAX - 0.5f) * 0.05f;
        particles[i].vz = ((float)rand() / RAND_MAX - 0.5f) * 0.05f;
        particles[i].life = 1.0f;
    }

    // Create particle buffer
    printf("Creating particle buffer...\n");
    GLuint particleBuffer;
    glGenBuffers(1, &particleBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, GPU_PARTICLES * sizeof(Particle), particles, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffer);
    checkGLError("After creating particle buffer");

    // Create triangle buffer for per triangle collision
    printf("Creating triangle buffer...\n");
    int numTriangles = 0;
    GPUTriangle* triangleData = createTriangleBuffer(&carModel, modelScale, offsetX, offsetY, offsetZ, carRotationY, &numTriangles);

    GLuint triangleBuffer = 0;
    if (triangleData && numTriangles > 0) {
        glGenBuffers(1, &triangleBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangleBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, numTriangles * sizeof(GPUTriangle), triangleData, GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, triangleBuffer);
        printf("Uploaded %d triangles to GPU\n", numTriangles);
    }

    // Create VAO for particles
    GLuint particleVAO;
    glGenVertexArrays(1, &particleVAO);
    glBindVertexArray(particleVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, particleBuffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)0);
    
    glBindVertexArray(0);
    checkGLError("After creating VAO");

    // Initialize fluid simulation
    printf("Creating fluid cube...\n");
    FluidCube* fluidCube = NULL;
    if (carModel.vertexCount > 0) {
        fluidCube = FluidCubeCreate(WIDTH / 10, HEIGHT / 10, 20, 0.001f, 0.0f, 0.001f, &carModel);
    }

    printf("Initialization complete. Starting main loop...\n");
    printf("\nCONTROLS\n");
    printf("Mouse drag:     Rotate camera\n");
    printf("Scroll wheel:   Zoom in/out\n");
    printf("A/D:            Rotate left/right\n");
    printf("W/S:            Rotate up/down\n");
    printf("Q/E:            Zoom in/out\n");
    printf("R:              Reset camera\n");
    printf("UP/DOWN:        Adjust wind speed\n");
    printf("0:              Collision OFF\n");
    printf("1:              AABB collision (fast)\n");
    printf("2:              Per-triangle collision (accurate)\n");
    printf("ESC:            Quit\n");

    // Collision mode: 0=off, 1=AABB, 2=per-triangle
    int collisionMode = 1;

    // Main loop
    int running = 1;
    Uint32 lastTime = SDL_GetTicks();
    int frameCount = 0;
    
    while (running) {
        frameCount++;
        
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } 
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    mouseDown = 1;
                    lastMouseX = event.button.x;
                    lastMouseY = event.button.y;
                }
            }
            else if (event.type == SDL_MOUSEBUTTONUP) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    mouseDown = 0;
                }
            }
            else if (event.type == SDL_MOUSEMOTION) {
                if (mouseDown) {
                    int dx = event.motion.x - lastMouseX;
                    int dy = event.motion.y - lastMouseY;
                    
                    cameraAngleY += dx * 0.005f;
                    cameraAngleX += dy * 0.005f;
                    
                    if (cameraAngleX > 1.5f) cameraAngleX = 1.5f;
                    if (cameraAngleX < -1.5f) cameraAngleX = -1.5f;
                    
                    lastMouseX = event.motion.x;
                    lastMouseY = event.motion.y;
                }
            }
            else if (event.type == SDL_MOUSEWHEEL) {
                cameraDistance -= event.wheel.y * 0.5f;
                if (cameraDistance < 1.0f) cameraDistance = 1.0f;
                if (cameraDistance > 20.0f) cameraDistance = 20.0f;
            }
            else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        running = 0;
                        break;
                    case SDLK_0:
                        collisionMode = 0;
                        printf("Collision: OFF\n");
                        break;
                    case SDLK_1:
                        collisionMode = 1;
                        printf("Collision: AABB (fast)\n");
                        break;
                    case SDLK_2:
                        collisionMode = 2;
                        printf("Collision: Per-Triangle (accurate) - %d triangles\n", numTriangles);
                        break;
                    case SDLK_UP:
                        windSpeed += 0.5f;
                        printf("Wind speed: %.1f\n", windSpeed);
                        break;
                    case SDLK_DOWN:
                        windSpeed -= 0.5f;
                        if (windSpeed < 0.0f) windSpeed = 0.0f;
                        printf("Wind speed: %.1f\n", windSpeed);
                        break;
                    case SDLK_a:
                        cameraAngleY -= 0.1f;
                        break;
                    case SDLK_d:
                        cameraAngleY += 0.1f;
                        break;
                    case SDLK_w:
                        cameraAngleX -= 0.1f;
                        if (cameraAngleX < -1.5f) cameraAngleX = -1.5f;
                        break;
                    case SDLK_s:
                        cameraAngleX += 0.1f;
                        if (cameraAngleX > 1.5f) cameraAngleX = 1.5f;
                        break;
                    case SDLK_q:
                        cameraDistance -= 0.5f;
                        if (cameraDistance < 1.0f) cameraDistance = 1.0f;
                        break;
                    case SDLK_e:
                        cameraDistance += 0.5f;
                        if (cameraDistance > 20.0f) cameraDistance = 20.0f;
                        break;
                    case SDLK_r:
                        cameraAngleY = 0.0f;
                        cameraAngleX = 0.3f;
                        cameraDistance = 6.0f;
                        printf("Camera reset\n");
                        break;
                }
            }
        }

        // Update view matrix
        calculateViewMatrix(view, cameraAngleY, cameraAngleX, cameraDistance,
                            cameraTargetX, cameraTargetY, cameraTargetZ);

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
        
        if (carMinLoc != -1) {
            glUniform3f(carMinLoc, carBounds.minX, carBounds.minY, carBounds.minZ);
        }
        if (carMaxLoc != -1) {
            glUniform3f(carMaxLoc, carBounds.maxX, carBounds.maxY, carBounds.maxZ);
        }
        if (carCenterLoc != -1) {
            glUniform3f(carCenterLoc, carBounds.centerX, carBounds.centerY, carBounds.centerZ);
        }
        if (collisionModeLoc != -1) {
            glUniform1i(collisionModeLoc, collisionMode);
        }
        if (numTrianglesLoc != -1) {
            glUniform1i(numTrianglesLoc, numTriangles);
        }
        
        glDispatchCompute((GPU_PARTICLES + 255) / 256, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        // Render particles
        glUseProgram(particleShaderProgram);
        if (viewLoc != -1) {
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
        }
        glBindVertexArray(particleVAO);
        glDrawArrays(GL_POINTS, 0, GPU_PARTICLES);
        checkGLError("After rendering particles");
        
        // Render car model
        if (carModel.faceCount > 0) {
            glUseProgram(0);
            
            glMatrixMode(GL_PROJECTION);
            glLoadMatrixf(projection);
            
            glMatrixMode(GL_MODELVIEW);
            glLoadMatrixf(view);
            
            renderModel(&carModel, SCALE);
            checkGLError("After rendering model");
        }

        // Debug output
        if (frameCount % 60 == 0) {
            const char* modeNames[] = {"OFF", "AABB", "TRI"};
            printf("Frame %d - FPS: %.1f - Collision: %s - Wind: %.1f\n", 
                   frameCount, 1.0f / deltaTime,
                   modeNames[collisionMode],
                   windSpeed);
        }

        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    printf("Cleaning up...\n");
    free(particles);
    if (triangleData) free(triangleData);
    freeModel(&carModel);
    if (fluidCube) FluidCubeFree(fluidCube);
    glDeleteVertexArrays(1, &particleVAO);
    glDeleteBuffers(1, &particleBuffer);
    if (triangleBuffer) glDeleteBuffers(1, &triangleBuffer);
    glDeleteProgram(particleShaderProgram);
    glDeleteProgram(computeShaderProgram);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    printf("Cleanup complete. Exiting.\n");
    return 0;
}