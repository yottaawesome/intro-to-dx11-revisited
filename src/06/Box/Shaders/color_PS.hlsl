// Outputs the interpolated vertex color.

#include "color.hlsli"

float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
}
