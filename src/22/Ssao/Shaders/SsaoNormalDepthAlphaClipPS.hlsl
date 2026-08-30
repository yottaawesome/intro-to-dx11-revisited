#include "SsaoNormalDepthCommon.hlsli"

Texture2D gDiffuseMap : register(t0);

SamplerState samLinear : register(s0);

float4 main(VertexOut pin) : SV_Target
{
    float4 textureColor = gDiffuseMap.Sample(samLinear, pin.Tex);
    clip(textureColor.a - 0.1f);

    pin.NormalV = normalize(pin.NormalV);
    return float4(pin.NormalV, pin.PosV.z);
}
