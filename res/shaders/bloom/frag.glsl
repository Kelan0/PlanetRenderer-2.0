#version 330 core

in vec2 fs_texturePosition;

uniform vec2 screenResolution;
uniform sampler2DMS glowTexture;

uniform bool copyPhase;

out vec4 outColour;

void main(void) {
    ivec2 texelPosition = ivec2(fs_texturePosition * screenResolution);
    
    if (copyPhase) {
        
    } else {

    }
}