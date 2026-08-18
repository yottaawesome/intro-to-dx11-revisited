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
    float3 gPadding;
    float4 gFogColor;
};

cbuffer cbPerObject : register(b1)
{
    row_major float4x4 gWorld;
    row_major float4x4 gWorldInvTranspose;
    row_major float4x4 gViewProj;
    row_major float4x4 gTexTransform;
    Material gMaterial;
}; 

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Tex : TEXCOORD;
    row_major float4x4 World : WORLD;
    float4 Color : COLOR;
    uint InstanceId : SV_InstanceID;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 Tex : TEXCOORD;
    float4 Color : COLOR;
};