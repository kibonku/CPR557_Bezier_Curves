#version 450

// Only location and type (vec3) matter, the name doesn't need to match

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUBO
{
    mat4 projectionViewMatrix;
    vec4 ambientLightColor; // w is intensity
    vec3 lightPosition;
    vec4 lightColor;
} ubo;

layout(push_constant) uniform Pushdata
{
    mat4 modelMatrix;
    mat4 normalMatrix;
} pushdata;


void main()
{
    vec3 directionToLight = ubo.lightPosition - fragPosWorld;
    //float attenuation = 1.0 / dot(directionToLight, directionToLight); // distance squared

    float attenuation = 0.8;

    vec3 lightColor = ubo.lightColor.xyz * ubo.lightColor.w * attenuation;
    vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 diffuseLight = lightColor * max(dot(normalize(fragNormalWorld), normalize(directionToLight)), 0); // make sure to normalize again for fragNormalWorld
    outColor = vec4((diffuseLight + ambientLight) * fragColor, 1.0);

    if (outColor.r < 0.001) outColor.r = 0.001;
    if (outColor.g < 0.001) outColor.g = 0.001;
    if (outColor.b < 0.001) outColor.b = 0.001;
}

