#include "SkyCommon.hlsli"

TextureCube gCubeMap : register(t0);
SamplerState samTriLinear : register(s0);

float4 main(SkyVertexOut pin) : SV_Target
{
    return gCubeMap.Sample(samTriLinear, pin.PosL);
}
