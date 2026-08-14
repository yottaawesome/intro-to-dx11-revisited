#include "common.hlsli"

// The domain shader is called for every vertex created by the tessellator.  
// It is like the vertex shader after tessellation.
[domain("quad")]
DomainOut main(PatchTess patchTess,
			 float2 uv : SV_DomainLocation,
			 const OutputPatch<HullOut, 4> quad)
{
	DomainOut dout;
	
	// Bilinear interpolation.
	float3 v1 = lerp(quad[0].PosL, quad[1].PosL, uv.x);
	float3 v2 = lerp(quad[2].PosL, quad[3].PosL, uv.x);
	float3 p = lerp(v1, v2, uv.y);
	
	// Displacement mapping
	p.y = 0.3f * (p.z * sin(p.x) + p.x * cos(p.z));
	
	dout.PosH = mul(float4(p, 1.0f), gWorldViewProj);
	
	return dout;
}