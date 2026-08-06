#include "color.hlsli"

float4 main(VertexOut pin) : SV_Target
{
    return pin.Color;
}
