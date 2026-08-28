cbuffer cbPerObject : register(b0)
{
    row_major float4x4 gWorldViewProj;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float AmbientOcc : AMBIENT;
};
