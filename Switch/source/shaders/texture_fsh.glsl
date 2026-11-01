#version 460

// Bilinear upscaler
layout (location = 0) in vec2 vUv;
layout (location = 0) out vec4 outColor;

layout (binding = 0) uniform sampler2D tex;

void main()
{
    outColor = texture(tex, vUv);
}
