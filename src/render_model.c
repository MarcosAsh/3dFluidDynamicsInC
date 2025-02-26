#include "../lib/render_model.h"
#include <GL/glew.h>

void renderModel(Model* model, int scale) {
    // Use OpenGL to render the model
    glBegin(GL_TRIANGLES); // Start drawing triangles

    for (int i = 0; i < model->faceCount; i++) {
        // Get the vertices for the current face
        Vertex v1 = model->vertices[model->faces[i].v1];
        Vertex v2 = model->vertices[model->faces[i].v2];
        Vertex v3 = model->vertices[model->faces[i].v3];

        // Scale and translate vertices to fit the simulation space
        float x1 = (v1.x + 1.0f) * 0.5f * scale;
        float y1 = (v1.y + 1.0f) * 0.5f * scale;
        float z1 = (v1.z + 1.0f) * 0.5f * scale;

        float x2 = (v2.x + 1.0f) * 0.5f * scale;
        float y2 = (v2.y + 1.0f) * 0.5f * scale;
        float z2 = (v2.z + 1.0f) * 0.5f * scale;

        float x3 = (v3.x + 1.0f) * 0.5f * scale;
        float y3 = (v3.y + 1.0f) * 0.5f * scale;
        float z3 = (v3.z + 1.0f) * 0.5f * scale;

        // Draw the triangle
        glVertex3f(x1, y1, z1);
        glVertex3f(x2, y2, z2);
        glVertex3f(x3, y3, z3);
    }

    glEnd(); // End drawing triangles
}