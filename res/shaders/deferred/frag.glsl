#version 330 core

const vec3 directionToLight = vec3(0.26726, 0.8018, 0.5345); // [1, 3, 2]

in vec2 fs_texturePosition;

uniform vec3 cameraPosition;
uniform float scaleFactor;
uniform int msaaSamples;
uniform vec2 screenResolution;
uniform sampler2DMS albedoTexture;
uniform sampler2DMS glowTexture;
uniform sampler2DMS normalTexture;
uniform sampler2DMS positionTexture;
uniform sampler2DMS specularEmissionTexture;
uniform sampler2DMS depthTexture;

out vec4 outColour;

void main(void) {
    ivec2 texelPosition = ivec2(fs_texturePosition * screenResolution);
    vec3 worldPosition = texelFetch(positionTexture, texelPosition, 0).xyz / scaleFactor;
    vec3 directionToCamera = -worldPosition; // World position is the position relative to the camera.
    float distanceToCamera = length(directionToCamera);
    directionToCamera /= distanceToCamera;
    
    vec3 albedo = texelFetch(albedoTexture, texelPosition, 0).rgb;
    vec3 normal = texelFetch(normalTexture, texelPosition, 0).xyz;
    vec3 specularEmission = texelFetch(specularEmissionTexture, texelPosition, 0).rgb;

    vec3 colour = albedo;

    // TODO: specular/emission, multiple lights, MSAA
    if (distanceToCamera > 0.0) {
        float diffuseCoef = clamp(dot(normal, directionToLight), 0.0, 1.0);
        vec3 diffuse = albedo * diffuseCoef;

        float specularCoef = 0.0;
        if (specularEmission.r > 0.0 && specularEmission.g > 0.0) {
            specularCoef = pow(max(0.0, dot(directionToCamera, normalize(reflect(-directionToLight, normal)))), specularEmission.r);
        }
        vec3 specular = albedo * diffuseCoef * specularCoef * specularEmission.g;

        colour = diffuse + specular;
    }

    outColour = vec4(colour, 1.0);
}