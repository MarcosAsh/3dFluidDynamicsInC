#version 430 core
out vec4 FragColor;

void main() {
    // Color based on depth (blue = far, white = near)
    float depth = gl_FragCoord.z;
    vec3 color = mix(vec3(0.5, 0.7, 1.0), vec3(1.0), 1.0 - depth);
    FragColor = vec4(color, 1.0);
}
