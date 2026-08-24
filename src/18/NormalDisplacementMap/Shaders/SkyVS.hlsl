//=============================================================================
// Sky.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Effect used to shade sky dome.
//=============================================================================

#include "SkyCommon.hlsli"

cbuffer cbPerFrame : register(b0)
{
    row_major float4x4 gWorldViewProj;
};

struct VertexIn
{
    float3 PosL : POSITION;
};

VertexOut main(VertexIn vin)
{
    VertexOut vout;
	
	// Set z = w so that z/w = 1 (i.e., skydome always on far plane).
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj).xyww;
	
	// Use local vertex position as cubemap lookup vector.
    vout.PosL = vin.PosL;
	
    return vout;
}