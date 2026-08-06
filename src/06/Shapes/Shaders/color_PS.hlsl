//***************************************************************************************
// color.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

#include "color.hlsli"

float4 main(VertexOut pin) : SV_Target
{
    return pin.Color;
}
