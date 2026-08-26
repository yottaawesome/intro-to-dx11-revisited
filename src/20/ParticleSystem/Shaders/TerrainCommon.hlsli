#include "LightHelper.hlsli"

cbuffer cbPerFrame : register(b0)
{
    DirectionalLight gDirLights[3];

    float3 gEyePosW;
    uint gLightCount;

    bool gFogEnabled;
    float gFogStart;
    float gFogRange;
    float gMinDist;

    float gMaxDist;
    float gMinTess;
    float gMaxTess;
    float gTexelCellSpaceU;

    float4 gFogColor;

    float gTexelCellSpaceV;
    float gWorldCellSpace;
    float2 gPadding1;

    float2 gTexScale;
    float2 gPadding2;
    float4 gWorldFrustumPlanes[6];
};

cbuffer cbPerObject : register(b1)
{
    row_major float4x4 gViewProj;
    Material gMaterial;
};

Texture2DArray gLayerMapArray : register(t0);
Texture2D gBlendMap : register(t1);
Texture2D gHeightMap : register(t2);
SamplerState samLinear : register(s0);
SamplerState samHeightmap : register(s1);

struct TerrainVertexIn
{
    float3 PosL : POSITION;
    float2 Tex : TEXCOORD0;
    float2 BoundsY : TEXCOORD1;
};

struct TerrainVertexOut
{
    float3 PosW : POSITION;
    float2 Tex : TEXCOORD0;
    float2 BoundsY : TEXCOORD1;
};

struct TerrainDomainOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float2 Tex : TEXCOORD0;
    float2 TiledTex : TEXCOORD1;
};

struct TerrainPatchTess
{
    float EdgeTess[4] : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};

struct TerrainHullOut
{
    float3 PosW : POSITION;
    float2 Tex : TEXCOORD0;
};
