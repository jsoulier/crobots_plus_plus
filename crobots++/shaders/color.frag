struct Input
{
    float3 Color : TEXCOORD0;
};

float4 main(Input input) : SV_Target0
{
    return float4(input.Color, 1.0f);
}