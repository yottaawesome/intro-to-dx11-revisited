#include "AmbientOcclusionCommon.hlsli"

float4 main(VertexOut pin) : SV_Target
{
    return float4(pin.AmbientOcc.xxx, 1.0f);
}
