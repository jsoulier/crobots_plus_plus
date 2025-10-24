#ifndef SHADER_HLSL
#define SHADER_HLSL

float3 GetColor(uint inColor)
{
    float3 color;
    color.r = float((inColor >> 16) & 0xFFu) / 255.0f;
    color.g = float((inColor >> 8) & 0xFFu) / 255.0f;
    color.b = float((inColor >> 0) & 0xFFu) / 255.0f;
    return color;
}

#endif