#include "../lib/model_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Model loadOBJ(const char* filepath) {
  Model model = {0};
  FILE* file = fopen(filepath, "r");
  if (!file) {
    printf("Could not open file %s\n", filepath);
    return model;
  }

  // Count vertices anad faces
  char line[128];
  while (fgets(line, sizeof(line), file)) {
    if (line[0] == 'v' && line[1] == ' ') {
      model.vertexCount++;
    } else if (line[0] == 'f' && line[1] == ' ') {
      model.faceCount++;
    }
  }

  // Allocate memory
  model.vertices = (Vertex*)malloc(model.vertexCount * sizeof(Vertex));
  model.faces = (Face*)malloc(model.faceCount * sizeof(Face));

  // Read file again to extract data
  rewind(file);
  int vertexIndex = 0, faceIndex = 0;
  while (fgets(line, sizeof(line), file)) {
    if (line[0] == 'v' && line[1] == ' ') {
      // Parse vertex
      sscanf(line, "v %f %f %f", &model.vertices[faceIndex].x,
                                &model.vertices[vertexIndex].y,
                                &model.vertices[vertexIndex].z);
    } else if (line[0] == 'f' && line[1] == ' '){
      // Parse face (assuming triangles)
     sscanf(line, "f %d %d %d", &model.faces[faceIndex].v1,
                               &model.faces[vertexIndex].v2,
                               &model.faces[vertexIndex].v3);
     // OBJ indices start at 1, so subtract 1
     model.faces[faceIndex].v1--;
     model.faces[faceIndex].v2--;
     model.faces[faceIndex].v3--;
     faceIndex++;
    }
  }

  fclose(file);
  return model;
}

void freeModel(Model* model) {
  free(model->vertices);
  free(model->faces);
  model->vertexCount = 0;
  model->faceCount = 0;
}