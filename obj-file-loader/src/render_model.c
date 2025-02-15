#include "../lib/model_loader.h"
#include <SDL2/SDL.h>

void renderModel(SDL_Renderer* renderer, Model* model, int scale) {
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White color for the model

  for( int i = 0; i < model->faceCount; i++){
    Vertex v1 = model->vertices[model->faces[i].v1];
    Vertex v2 = model->vertices[model->faces[i].v2];
    Vertex v3 = model->vertices[model->faces[i].v3];

    // Scale and translate vertices to fit the simulation space
    int x1 = (int)((v1.x + 1.0f) * 0.5f * scale);
    int y1 = (int)((v1.y + 1.0f) * 0.5f * scale);
    int x2 = (int)((v2.x + 1.0f) * 0.5f * scale);
    int y2 = (int)((v2.y + 1.0f) * 0.5f * scale);
    int x3 = (int)((v3.x + 1.0f) * 0.5f * scale);
    int y3 = (int)((v3.y + 1.0f) * 0.5f * scale);

    // Draw the triangle edges
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    SDL_RenderDrawLine(renderer, x2, y2, x3, y3);
    SDL_RenderDrawLine(renderer, x3, y3, x1, y1);
  }
}
