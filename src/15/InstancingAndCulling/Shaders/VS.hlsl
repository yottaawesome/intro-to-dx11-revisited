#include "Common.hlsli"

VertexOut main(VertexIn vin)
{
    VertexOut vout;
	
	// Transform to world space space.
    vout.PosW = mul(float4(vin.PosL, 1.0f), vin.World).xyz;
    vout.NormalW = mul(vin.NormalL, (float3x3) vin.World);
		
	// Transform to homogeneous clip space.
    vout.PosH = mul(float4(vout.PosW, 1.0f), gViewProj);
	
	// Output vertex attributes for interpolation across triangle.
    vout.Tex = mul(float4(vin.Tex, 0.0f, 1.0f), gTexTransform).xy;
    vout.Color = vin.Color;

    return vout;
}