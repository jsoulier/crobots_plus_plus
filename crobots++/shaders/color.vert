#include "shader.hlsl"

cbuffer UniformBuffer : register(b0, space1)
{
    float4x4 ViewProj : packoffset(c0);
};

struct Input
{
    float3 Position : TEXCOORD0;
    uint Color : TEXCOORD1;
};

struct Output
{
    float4 Position : SV_Position;
    float3 Color : TEXCOORD0;
};

Output main(Input input)
{
    Output output;
    output.Position = mul(ViewProj, float4(input.Position, 1.0f));
    output.Color = GetColor(input.Color);
    return output;
}