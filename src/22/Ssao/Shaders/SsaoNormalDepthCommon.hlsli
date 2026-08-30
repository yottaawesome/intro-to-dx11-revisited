cbuffer cbPerObject : register(b0)
{
    row_major float4x4 gWorldView;
    row_major float4x4 gWorldInvTransposeView;
    row_major float4x4 gWorldViewProj;
    row_major float4x4 gTexTransform;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosV : POSITION;
    float3 NormalV : NORMAL;
    float2 Tex : TEXCOORD0;
};
