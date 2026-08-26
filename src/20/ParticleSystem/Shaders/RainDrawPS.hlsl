#include "RainCommon.hlsli"

float4 main(RainGeoOut input) : SV_Target
{
    return gTexArray.Sample(samLinear, float3(input.Tex, 0.0f));
}
