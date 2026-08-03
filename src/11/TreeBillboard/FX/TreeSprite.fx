//***************************************************************************************
// TreeSprite.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Uses the geometry shader to expand a point sprite into a y-axis aligned 
// billboard that faces the camera.
//***************************************************************************************

#include "LightHelper.fx"
 
cbuffer cbPerFrame
{
    DirectionalLight gDirLights[3];
    float3 gEyePosW;
    float gFogStart;
    float gFogRange;
    float3 gPadding;
    float4 gFogColor;
};

cbuffer cbPerObject
{
    row_major float4x4 gViewProj;
    Material gMaterial;
    int gLightCount;
    bool gUseTexture;
    bool gAlphaClip;
    bool gFogEnabled;
};

cbuffer cbFixed
{
	//
	// Compute texture coordinates to stretch texture over quad.
	//

    float2 gTexC[4] =
    {
        float2(0.0f, 1.0f),
		float2(0.0f, 0.0f),
		float2(1.0f, 1.0f),
		float2(1.0f, 0.0f)
    };
};

// Nonnumeric values cannot be added to a cbuffer.
Texture2DArray gTreeMapArray;

SamplerState samLinear : register(s0);

struct VertexIn
{
    float3 PosW : POSITION;
    float2 SizeW : SIZE;
};

struct VertexOut
{
    float3 CenterW : POSITION;
    float2 SizeW : SIZE;
};

struct GeoOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 Tex : TEXCOORD;
    uint PrimID : SV_PrimitiveID;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;

	// Just pass data over to geometry shader.
    vout.CenterW = vin.PosW;
    vout.SizeW = vin.SizeW;

    return vout;
}
 
 // We expand each point into a quad (4 vertices), so the maximum number of vertices
 // we output per geometry shader invocation is 4.
[maxvertexcount(4)]
void GS(point VertexOut gin[1],
        uint primID : SV_PrimitiveID,
        inout TriangleStream<GeoOut> triStream)
{
	//
	// Compute the local coordinate system of the sprite relative to the world
	// space such that the billboard is aligned with the y-axis and faces the eye.
	//

    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 look = gEyePosW - gin[0].CenterW;
    look.y = 0.0f; // y-axis aligned, so project to xz-plane
    look = normalize(look);
    float3 right = cross(up, look);

	//
	// Compute triangle strip vertices (quad) in world space.
	//
    float halfWidth = 0.5f * gin[0].SizeW.x;
    float halfHeight = 0.5f * gin[0].SizeW.y;
	
    float4 v[4];
    v[0] = float4(gin[0].CenterW + halfWidth * right - halfHeight * up, 1.0f);
    v[1] = float4(gin[0].CenterW + halfWidth * right + halfHeight * up, 1.0f);
    v[2] = float4(gin[0].CenterW - halfWidth * right - halfHeight * up, 1.0f);
    v[3] = float4(gin[0].CenterW - halfWidth * right + halfHeight * up, 1.0f);

	//
	// Transform quad vertices to world space and output 
	// them as a triangle strip.
	//
    GeoOut gout;
	[unroll]
    for (int i = 0; i < 4; ++i)
    {
        gout.PosH = mul(v[i], gViewProj);
        gout.PosW = v[i].xyz;
        gout.NormalW = look;
        gout.Tex = gTexC[i];
        gout.PrimID = primID;
		
        triStream.Append(gout);
    }
}

float4 PS(GeoOut pin) : SV_Target
{
	// Interpolating normal can unnormalize it, so normalize it.
    pin.NormalW = normalize(pin.NormalW);

	// The toEye vector is used in lighting.
    float3 toEye = gEyePosW - pin.PosW;

	// Cache the distance to the eye from this surface point.
    float distToEye = length(toEye);

	// Normalize.
    toEye /= distToEye;
   
    // Default to multiplicative identity.
    float4 texColor = float4(1, 1, 1, 1);
    if (gUseTexture)
    {
		// Sample texture.
        float3 uvw = float3(pin.Tex, pin.PrimID % 4);
        texColor = gTreeMapArray.Sample(samLinear, uvw);

        if (gAlphaClip)
        {
			// Discard pixel if texture alpha < 0.05.  Note that we do this
			// test as soon as possible so that we can potentially exit the shader 
			// early, thereby skipping the rest of the shader code.
            clip(texColor.a - 0.05f);
        }
    }

	//
	// Lighting.
	//

    float4 litColor = texColor;
    if (gLightCount > 0)
    {
		// Start with a sum of zero.
        float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

		// Sum the light contribution from each light source.  
		[unroll]
        for (int i = 0; i < gLightCount; ++i)
        {
            float4 A, D, S;
            ComputeDirectionalLight(gMaterial, gDirLights[i], pin.NormalW, toEye,
				A, D, S);

            ambient += A;
            diffuse += D;
            spec += S;
        }

		// Modulate with late add.
        litColor = texColor * (ambient + diffuse) + spec;
    }

	//
	// Fogging
	//

    if (gFogEnabled)
    {
        float fogLerp = saturate((distToEye - gFogStart) / gFogRange);

		// Blend the fog color and the lit color.
        litColor = lerp(litColor, gFogColor, fogLerp);
    }

	// Common to take alpha from diffuse material and texture.
    litColor.a = gMaterial.Diffuse.a * texColor.a;

    return litColor;
}
