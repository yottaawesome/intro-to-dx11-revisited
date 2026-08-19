#include "LightHelper.hlsli"

// Nonnumeric values cannot be added to a cbuffer.
Texture2D gDiffuseMap;

SamplerState samAnisotropic : register(s0);

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
    float3 gPadding;
    float4 gFogColor;
};

cbuffer cbPerObject : register(b1)
{
    row_major float4x4 gWorld;
    row_major float4x4 gWorldInvTranspose;
    row_major float4x4 gWorldViewProj;
    row_major float4x4 gTexTransform;
    Material gMaterial;
}; 

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 Tex : TEXCOORD;
};