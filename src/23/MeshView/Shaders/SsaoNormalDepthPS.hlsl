#include "SsaoNormalDepthCommon.hlsli"

float4 main(VertexOut pin) : SV_Target
{
    pin.NormalV = normalize(pin.NormalV);
    return float4(pin.NormalV, pin.PosV.z);
}
