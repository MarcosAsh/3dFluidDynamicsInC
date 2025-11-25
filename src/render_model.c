#include "../lib/render_model.h"
#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void renderModel(Model* model, int scale) {
    if (model->faceCount == 0) return;
    
    glPushMatrix();
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColor3f(1.0f, 1.0f, 0.0f);  // Yellow
    
    glBegin(GL_TRIANGLES);
    // Render all faces now
    for (int i = 0; i < model->faceCount; i++) {
        Vertex v1 = model->vertices[model->faces[i].v1];
        Vertex v2 = model->vertices[model->faces[i].v2];
        Vertex v3 = model->vertices[model->faces[i].v3];
        
        // Scale up more and center it
        float scaleF = 0.05f;  // Bigger scale
        float offsetX = 0.0f;
        float offsetY = -0.2f;  // Move down a bit
        float offsetZ = -0.9f;  // Move closer to camera
        
        glVertex3f(v1.x * scaleF + offsetX, v1.y * scaleF + offsetY, v1.z * scaleF + offsetZ);
        glVertex3f(v2.x * scaleF + offsetX, v2.y * scaleF + offsetY, v2.z * scaleF + offsetZ);
        glVertex3f(v3.x * scaleF + offsetX, v3.y * scaleF + offsetY, v3.z * scaleF + offsetZ);
    }
    glEnd();
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPopMatrix();
}