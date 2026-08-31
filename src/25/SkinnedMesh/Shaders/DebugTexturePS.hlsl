#include "DebugTextureCommon.hlsli"

Texture2D gTexture : register(t0);

SamplerState samLinear : register(s0);

float4 main(VertexOut pin) : SV_Target
{
    return gTexture.Sample(samLinear, pin.Tex);
}
