cbuffer UniformBuffer : register(b0, space1)
{
    float4x4 ViewProj : packoffset(c0);
};

struct Input
{
    float3 Position : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float4x4 Transform : TEXCOORD2;
};

struct Output
{
    float4 Position : SV_Position;
    float3 Normal : TEXCOORD0;
};

Output main(Input input)
{
    Output output;
    output.Position = mul(mul(float4(input.Position, 1.0f), input.Transform), ViewProj);
    output.Normal = mul(input.Normal, (float3x3) input.Transform);
    return output;
}