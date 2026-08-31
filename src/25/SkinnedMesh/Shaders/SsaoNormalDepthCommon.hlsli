cbuffer cbPerObject : register(b0)
{
    row_major float4x4 gWorldView;
    row_major float4x4 gWorldInvTransposeView;
    row_major float4x4 gWorldViewProj;
    row_major float4x4 gTexTransform;
};

cbuffer cbSkinned : register(b1)
{
    row_major float4x4 gBoneTransforms[96];
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosV : POSITION;
    float3 NormalV : NORMAL;
    float2 Tex : TEXCOORD0;
};
