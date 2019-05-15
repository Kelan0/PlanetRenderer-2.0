#version 330 core

in vec4 fs_colour;

out vec4 outColour;

void main(void) {
    outColour = fs_colour;
}