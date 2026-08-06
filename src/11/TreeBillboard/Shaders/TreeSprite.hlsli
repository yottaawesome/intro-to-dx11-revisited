#include "LightHelper.hlsli"

cbuffer cbPerFrame
{
    DirectionalLight gDirLights[3];
    float3 gEyePosW;
    float gFogStart;
    float gFogRange;
    int gLightCount;
    bool gUseTexture;
    bool gAlphaClip;
    bool gFogEnabled;
    float3 gPadding;
    float4 gFogColor;
};

cbuffer cbPerObject
{
    row_major float4x4 gViewProj;
    Material gMaterial;
};

//
// Compute texture coordinates to stretch texture over the quad.
//
static const float2 gTexC[4] =
{
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 1.0f),
    float2(1.0f, 0.0f)
};

// Nonnumeric values cannot be added to a cbuffer.
Texture2DArray gTreeMapArray;

SamplerState samLinear : register(s0);

struct VertexIn
{
    float3 PosW : POSITION;
    float2 SizeW : SIZE;
};

struct VertexOut
{
    float3 CenterW : POSITION;
    float2 SizeW : SIZE;
};

struct GeoOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 Tex : TEXCOORD;
    uint PrimID : SV_PrimitiveID;
};
