cbuffer cbPerFrame : register(b0)
{
    float3 gEyePosW;
    float gHeightScale;
    float gMaxTessDistance;
    float gMinTessDistance;
    float gMinTessFactor;
    float gMaxTessFactor;
};

cbuffer cbPerObject : register(b1)
{
    row_major float4x4 gWorld;
    row_major float4x4 gWorldInvTranspose;
    row_major float4x4 gViewProj;
    row_major float4x4 gWorldViewProj;
    row_major float4x4 gTexTransform;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 Tex : TEXCOORD;
};

struct TessVertexOut
{
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 Tex : TEXCOORD;
    float TessFactor : TESS;
};

struct PatchTess
{
    float EdgeTess[3] : SV_TessFactor;
    float InsideTess : SV_InsideTessFactor;
};

struct HullOut
{
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 Tex : TEXCOORD;
};

struct DomainOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 Tex : TEXCOORD;
};
