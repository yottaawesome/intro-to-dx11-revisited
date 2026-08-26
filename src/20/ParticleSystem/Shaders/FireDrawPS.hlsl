#include "FireCommon.hlsli"

float4 main(FireGeoOut input) : SV_Target
{
    return gTexArray.Sample(samLinear, float3(input.Tex, 0.0f)) * input.Color;
}
