#include "LightHelper.hlsli"
 
cbuffer cbPerFrame : register(b0)
{
    DirectionalLight gDirLights[3];
    
    float3 gEyePosW;
    int gLightCount;
    
    bool gFogEnabled;
    float gFogStart;
    float gFogRange;
	// When distance is minimum, the tessellation is maximum.
	// When distance is maximum, the tessellation is minimum.
    float gMinDist;
    
    float gMaxDist;
    // Exponents for power of 2 tessellation.  The tessellation
	// range is [2^(gMinTess), 2^(gMaxTess)].  Since the maximum
	// tessellation is 64, this means gMaxTess can be at most 6
	// since 2^6 = 64.
    float gMinTess;
    float gMaxTess;
    float gTexelCellSpaceU;
    
    float4 gFogColor;
	
    float gTexelCellSpaceV;
    float gWorldCellSpace;
    float2 padding1;
    
    float2 gTexScale = 50.0f;
    float2 padding2;
    float4 gWorldFrustumPlanes[6];
};

cbuffer cbPerObject : register(b1)
{
	// Terrain coordinate specified directly 
	// at center of world space.
	
    row_major float4x4 gViewProj;
    Material gMaterial;
};

// Nonnumeric values cannot be added to a cbuffer.
Texture2DArray gLayerMapArray;
Texture2D gBlendMap;
Texture2D gHeightMap;

SamplerState samLinear : register(s0);

SamplerState samHeightmap : register(s1);

struct VertexIn
{
    float3 PosL : POSITION;
    float2 Tex : TEXCOORD0;
    float2 BoundsY : TEXCOORD1;
};

struct VertexOut
{
    float3 PosW : POSITION;
    float2 Tex : TEXCOORD0;
    float2 BoundsY : TEXCOORD1;
};

struct DomainOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float2 Tex : TEXCOORD0;
    float2 TiledTex : TEXCOORD1;
};

struct PatchTess
{
    float EdgeTess[4] : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};

struct HullOut
{
    float3 PosW : POSITION;
    float2 Tex : TEXCOORD0;
};
