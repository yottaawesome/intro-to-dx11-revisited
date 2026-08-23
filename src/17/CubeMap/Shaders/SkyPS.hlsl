//=============================================================================
// Sky.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Effect used to shade sky dome.
//=============================================================================

#include "SkyCommon.hlsli"

// Nonnumeric values cannot be added to a cbuffer.
TextureCube gCubeMap;

SamplerState samTriLinearSam : register(s0);

float4 main(VertexOut pin) : SV_Target
{
    return gCubeMap.Sample(samTriLinearSam, pin.PosL);
}