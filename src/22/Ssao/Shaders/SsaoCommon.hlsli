cbuffer cbPerFrame : register(b0)
{
    row_major float4x4 gViewToTexSpace;
    float4 gOffsetVectors[14];
    float4 gFrustumCorners[4];
    float gOcclusionRadius;
    float gOcclusionFadeStart;
    float gOcclusionFadeEnd;
    float gSurfaceEpsilon;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 ToFarPlane : TEXCOORD0;
    float2 Tex : TEXCOORD1;
};
