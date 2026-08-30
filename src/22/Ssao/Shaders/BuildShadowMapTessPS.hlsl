#include "BuildShadowMapCommon.hlsli"

Texture2D gDiffuseMap : register(t0);

SamplerState samLinear : register(s0);

void main(DomainOut pin)
{
    float4 diffuse = gDiffuseMap.Sample(samLinear, pin.Tex);
    clip(diffuse.a - 0.15f);
}
