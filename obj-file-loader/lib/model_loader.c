#include "../lib/model_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int countVerticesInFile(const char* filePath) {
  FILE* file = fopen(filePath, "r");
  if (!file) {
    printf("Error: Could not open file %s\n", filePath); // Debug print
    return 0;
  }

  int vertexCount = 0;
  char line[256];

  while (fgets(line, sizeof(line), file)) {
    if (line[0] == 'v' && line[1] == ' ') {
        vertexCount++;
    }
  }
  fclose(file);
  return vertexCount;
}

int countFacesInFile(const char* filePath) {
  FILE* file = fopen(filePath, "r");
  if (!file) {
    printf("Error: could not open file %s\n", filePath); // Debug print
    return 0;
  }

  int faceCount = 0;
  char line[256];

  while(fgets(line, sizeof(line), file)) {
    if (line[0] == 'f' && line[1] == ' ') {
      faceCount++;
    }
  }

  fclose(file);
  return faceCount;
}

Model loadOBJ(const char* filePath) {
  Model model = {0};

  // Count vertices and faces
  int vertexCount = countVerticesInFile(filePath);
  int faceCount = countFacesInFile(filePath);

  printf("File contains %d vertices and %d faces.\n", vertexCount, faceCount);

  // Allocate memory
  model.vertices = (Vertex*)malloc(vertexCount * sizeof(Vertex));
  model.faces = (Face*)malloc(faceCount * sizeof(Face));

  if (!model.vertices || !model.faces) {
    printf("Error: Out of memory!\n");
    free(model.vertices);
    free(model.faces);
    return model;
  }

  // Load data
  FILE* file = fopen(filePath, "r");
  if (!file) {
    printf("Error: Could not open file %s\n", filePath);
    free(model.vertices);
    free(model.faces);
    return model;
  }

  char line[256];
  int vertexIndex = 0, faceIndex = 0;

  while (fgets(line, sizeof(line), file)) {
    if (line[0] == 'v' && line[1] == ' ') {
      // Parse vertex
      sscanf(line, "v %f %f %f", &model.vertices[vertexIndex].x,
                                &model.vertices[vertexIndex].y,
                                &model.vertices[vertexIndex].z);
      vertexIndex++;
    } else if (line[0] == 'f' && line[1] == ' ') {
      // Parse face
      sscanf(line, "f %d %d %d", &model.faces[faceIndex].v1,
                                &model.faces[faceIndex].v2,
                                &model.faces[faceIndex].v3);
      faceIndex++;
    }
  }

  fclose(file);

  model.vertexCount = vertexCount;
  model.faceCount = faceCount;

  printf("Successfully loaded model with %d vertices and %d faces.\n", vertexCount, faceCount);

  printf("Model loaded: %d vertices, %d faces\n", model.vertexCount, model.faceCount);
  if (model.vertexCount == 0 || model.faceCount == 0) {
    printf("Error: Model contains no vertices or faces.\n");
  }

  return model;
}

void freeModel(Model* model) {
  free(model->vertices);
  free(model->faces);
  model->vertexCount = 0;
  model->faceCount = 0;
}

// Moller-Trumbore ray-triangle intersection algorithm
int rayTriangleIntersection(Vertex rayOrigin, Vertex rayDirection, Vertex v0, Vertex v1, Vertex v2, float* t) {
  Vertex edge1, edge2, h, s, q;
  float a, f, u, v;

  edge1.x = v1.x - v0.x;
  edge1.y = v1.y - v0.y;
  edge1.z = v1.z - v0.z;

  edge2.x = v2.x - v0.x;
  edge2.y = v2.y - v0.y;
  edge2.z = v2.z - v0.z;

  h.x = rayDirection.y * edge2.z - rayDirection.z * edge2.y;
  h.y = rayDirection.z * edge2.x - rayDirection.x * edge2.z;
  h.z = rayDirection.x * edge2.y - rayDirection.y * edge2.x;

  a = edge1.x * h.x + edge1.y * h.y + edge1.z * h.z;

  if (a > -0.00001f && a < 0.00001f) {
    return 0; // Ray is parallel to the triangle
  }

  f = 1.0f / a;
  s.x = rayOrigin.x - v0.x;
  s.y = rayOrigin.y - v0.y;
  s.z = rayOrigin.z - v0.z;

  u = f * (s.x * h.x + s.y * h.y + s.z * h.z);

  if (u < 0.0f || u > 1.0f) {
    return 0; // Intersection point is outside the triangle
  }

  q.x = s.y * edge1.z - s.z * edge1.y;
  q.y = s.z * edge1.x - s.x * edge1.z;
  q.z = s.x * edge1.y - s.y * edge1.x;

  v = f * (rayDirection.x * q.x + rayDirection.y * q.y + rayDirection.z * q.z);

  if (v < 0.0f || u+v > 1.0f){
    return 0; // Intersection point is outside the traingle
  }

  *t = f * (edge2.x * q.x + edge2.y * q.y + edge2.z * q.z);

  if (*t > 0.00001f) {
   return 1; // Intersection found
  }

  return 0; // Intersection is behind the ray origin
}

int isInsideCarModel(int x, int y, int z, Model* model, int sizeX, int sizeY, int sizeZ) {
  // Scale grid coordinates to match the model's coordinate system
  float scaledX = (float)x / sizeX * 2.0f - 1.0f; // Map to [-1, 1]
  float scaledY = (float)y / sizeY * 2.0f - 1.0f;
  float scaledZ = (float)z / sizeZ * 2.0f - 1.0f;

  // Define the ray origin and direction
  Vertex rayOrigin = {scaledX, scaledY, -2.0f}; // Ray starts outside the model
  Vertex rayDirection = {0.0f, 0.0f, 1.0f};

  int intersectionCount = 0;

  // Test the ray against all triangles in the model
  for (int i = 0; i < model->faceCount; i++) {
    Vertex v0 = model->vertices[model->faces[i].v1];
    Vertex v1 = model->vertices[model->faces[i].v2];
    Vertex v2 = model->vertices[model->faces[i].v3];

    float t;
    if (rayTriangleIntersection(rayOrigin, rayDirection, v0, v1, v2, &t)) {
      intersectionCount++;
    }
  }

  // If the number of intersections is odd, the point is inside the model
  return (intersectionCount % 2 == 1);
}