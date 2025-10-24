#include "shader.hlsl"

cbuffer UniformBuffer : register(b0, space1)
{
    float4x4 ViewProj : packoffset(c0);
};

struct Input
{
    float3 Position : TEXCOORD0;
    uint Color : TEXCOORD1;
    float2 WorldPosition : TEXCOORD2;
    float Sin : TEXCOORD3;
    float Cos : TEXCOORD4;
};

struct Output
{
    float4 Position : SV_Position;
    float3 Color : TEXCOORD0;
};

Output main(Input input)
{
    float3 position;
    position.x = input.Position.x * input.Cos - input.Position.z * input.Sin;
    position.y = input.Position.y;
    position.z = input.Position.x * input.Sin + input.Position.z * input.Cos;
    position.x += input.WorldPosition.x;
    position.z += input.WorldPosition.y;
    Output output;
    output.Position = mul(ViewProj, float4(position, 1.0f));
    output.Color = GetColor(input.Color);
    return output;
}