#version 450 core
// Vertex
#ifdef VERTEX_SHADER
layout(location=0) in vec3 aPos;
uniform mat4 uViewProjection;
out vec3 WorldPos;
void main() {
    WorldPos    = aPos;
    gl_Position = uViewProjection * vec4(aPos, 1.0);
}
#endif

// Fragment
#ifdef FRAGMENT_SHADER
in  vec3 WorldPos;
out vec4 FragColor;
uniform vec3  uCamPos;
uniform float uGridSize;   // cell size
uniform vec3  uLineColor;

float Grid(vec2 p, float size) {
    vec2 g = abs(fract(p / size - 0.5) - 0.5) / fwidth(p / size);
    return 1.0 - min(min(g.x, g.y), 1.0);
}

void main() {
    float dist  = length(WorldPos.xz - uCamPos.xz);
    float alpha = 1.0 - smoothstep(30.0, 60.0, dist);
    float line  = Grid(WorldPos.xz, uGridSize);
    FragColor   = vec4(uLineColor, line * alpha * 0.6);
}
#endif
