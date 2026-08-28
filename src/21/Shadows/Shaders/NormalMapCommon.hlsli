#include "LightHelper.hlsli"

cbuffer cbPerFrame : register(b0)
{
    DirectionalLight gDirLights[3];
    float3 gEyePosW;
    float gFogStart;
    float gFogRange;
    uint gLightCount;
    bool gUseTexture;
    bool gAlphaClip;
    bool gFogEnabled;
    bool gReflectionEnabled;
    float2 gPadding;
    float4 gFogColor;
};

cbuffer cbPerObject : register(b1)
{
    row_major float4x4 gWorld;
    row_major float4x4 gWorldInvTranspose;
    row_major float4x4 gWorldViewProj;
    row_major float4x4 gTexTransform;
    row_major float4x4 gShadowTransform;
    Material gMaterial;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 Tex : TEXCOORD0;
    float4 ShadowPosH : TEXCOORD1;
};
