#version 430 core
layout(location = 0) in vec3 position;

uniform mat4 projection;
uniform mat4 view;

void main() {
    gl_Position = projection * view * vec4(position, 1.0);
    
    // Size varies with distance for depth perception
    float dist = length(gl_Position.xyz);
    gl_PointSize = 5.0 / (dist * 0.5);
}
