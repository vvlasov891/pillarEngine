#version 450 core

#define MAX_POINT_LIGHTS 8

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

// Material
uniform sampler2D uAlbedoMap;
uniform vec3      uColor;
uniform float     uShininess;

// Camera
uniform vec3 uCamPos;

// Directional light
uniform vec3  uDirLightDir;
uniform vec3  uDirLightColor;
uniform float uDirLightIntensity;

// Point lights
uniform int   uNumPointLights;
uniform vec3  uPointLightPos[MAX_POINT_LIGHTS];
uniform vec3  uPointLightColor[MAX_POINT_LIGHTS];
uniform float uPointLightIntensity[MAX_POINT_LIGHTS];
uniform float uPointLightRange[MAX_POINT_LIGHTS];

vec3 CalcDirLight(vec3 norm, vec3 viewDir, vec3 albedo) {
    vec3 lightDir = normalize(-uDirLightDir);
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 halfDir  = normalize(lightDir + viewDir);
    float spec    = pow(max(dot(norm, halfDir), 0.0), uShininess);
    vec3 ambient  = 0.15 * uDirLightColor * albedo;
    vec3 diffuse  = diff * uDirLightColor * albedo * uDirLightIntensity;
    vec3 specular = spec * uDirLightColor * 0.3 * uDirLightIntensity;
    return ambient + diffuse + specular;
}

vec3 CalcPointLight(int i, vec3 norm, vec3 viewDir, vec3 albedo) {
    vec3  toLight = uPointLightPos[i] - FragPos;
    float dist    = length(toLight);
    if (dist > uPointLightRange[i]) return vec3(0.0);
    vec3  lightDir = normalize(toLight);
    float atten    = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
    float diff     = max(dot(norm, lightDir), 0.0);
    vec3  halfDir  = normalize(lightDir + viewDir);
    float spec     = pow(max(dot(norm, halfDir), 0.0), uShininess);
    vec3 diffuse   = diff * uPointLightColor[i] * albedo * uPointLightIntensity[i] * atten;
    vec3 specular  = spec * uPointLightColor[i] * 0.3   * uPointLightIntensity[i] * atten;
    return diffuse + specular;
}

void main() {
    vec4 texColor = texture(uAlbedoMap, TexCoord);
    vec3 albedo   = texColor.rgb * uColor;
    vec3 norm     = normalize(Normal);
    vec3 viewDir  = normalize(uCamPos - FragPos);

    vec3 result = CalcDirLight(norm, viewDir, albedo);
    for (int i = 0; i < uNumPointLights && i < MAX_POINT_LIGHTS; ++i)
        result += CalcPointLight(i, norm, viewDir, albedo);

    FragColor = vec4(result, texColor.a);
}
