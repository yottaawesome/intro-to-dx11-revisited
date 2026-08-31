#include "DebugTextureCommon.hlsli"

Texture2D gTexture : register(t0);

SamplerState samLinear : register(s0);

float4 main(VertexOut pin) : SV_Target
{
    float value = gTexture.Sample(samLinear, pin.Tex).b;
    return float4(value.xxx, 1.0f);
}
