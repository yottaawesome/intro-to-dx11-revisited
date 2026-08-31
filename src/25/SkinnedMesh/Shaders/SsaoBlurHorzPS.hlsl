#include "SsaoBlurCommon.hlsli"

float4 main(VertexOut pin) : SV_Target
{
    return Blur(pin, float2(gTexelWidth, 0.0f));
}
