#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;

// Set and binding should match the descriptor set layout
layout(set = 0, binding = 0) uniform GlobalUBO
{
    mat4 projectionViewMatrix;
    vec4 ambientLightColor; // w is intensity
    vec3 lightPosition;
    vec4 lightColor;
} ubo;

// Note 128 bytes can only contain 2 4x4 matrices, so we run into the limitation
layout(push_constant) uniform Pushdata
{
    mat4 modelMatrix;
    mat4 normalMatrix;
} pushdata;


void main()
{
    vec4 positionWorld = pushdata.modelMatrix * vec4(position, 1.0);
    gl_Position = ubo.projectionViewMatrix * positionWorld;

    fragNormalWorld = normalize(mat3(pushdata.normalMatrix) * normal);
    fragPosWorld = positionWorld.xyz;
    fragColor = color;
}

