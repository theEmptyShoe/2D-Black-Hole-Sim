#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in float aAlpha;
uniform vec2 screenSize;
out float alpha;

void main() {
    alpha = aAlpha;
    vec2 pos = aPos;

    pos.x = pos.x / screenSize.x * 2.0 - 1.0;
    pos.y = 1.0 - pos.y / screenSize.y * 2.0;

    gl_Position = vec4(pos, 0.0, 1.0);
}
