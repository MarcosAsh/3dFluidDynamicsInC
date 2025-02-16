#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

typedef struct {
  float x, y, z;
} Vertex;

typedef struct {
  int v1, v2, v3;
} Face;

typedef struct {
  Vertex* vertices;
  Face* faces;
  int vertexCount;
  int faceCount;
} Model;

Model loadOBJ(const char* filePath);
void freeModel(Model* model);

// Declare the isInsideCarModel function
int isInsideCarModel(int x, int y, int z, Model* model, int sizeX, int sizeY, int sizeZ);

#endif // MODEL_LOADER_H