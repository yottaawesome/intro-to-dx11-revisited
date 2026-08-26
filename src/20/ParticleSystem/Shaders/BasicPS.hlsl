#include "BasicCommon.hlsli"

Texture2D gDiffuseMap : register(t0);
TextureCube gCubeMap : register(t1);
SamplerState samAnisotropic : register(s0);

float4 main(BasicVertexOut pin) : SV_Target
{
    pin.NormalW = normalize(pin.NormalW);

    float3 toEye = gEyePosW - pin.PosW;
    float distToEye = length(toEye);
    toEye /= distToEye;

    float4 texColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (gUseTexture)
    {
        texColor = gDiffuseMap.Sample(samAnisotropic, pin.Tex);
        if (gAlphaClip)
        {
            clip(texColor.a - 0.1f);
        }
    }

    float4 litColor = texColor;
    if (gLightCount > 0)
    {
        float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

        [unroll]
        for (uint i = 0; i < gLightCount; ++i)
        {
            float4 lightAmbient;
            float4 lightDiffuse;
            float4 lightSpec;
            ComputeDirectionalLight(
                gMaterial,
                gDirLights[i],
                pin.NormalW,
                toEye,
                lightAmbient,
                lightDiffuse,
                lightSpec);

            ambient += lightAmbient;
            diffuse += lightDiffuse;
            spec += lightSpec;
        }

        litColor = texColor * (ambient + diffuse) + spec;

        if (gReflectionEnabled)
        {
            float3 reflectionVector = reflect(-toEye, pin.NormalW);
            float4 reflectionColor = gCubeMap.Sample(samAnisotropic, reflectionVector);
            litColor += gMaterial.Reflect * reflectionColor;
        }
    }

    if (gFogEnabled)
    {
        float fogLerp = saturate((distToEye - gFogStart) / gFogRange);
        litColor = lerp(litColor, gFogColor, fogLerp);
    }

    litColor.a = gMaterial.Diffuse.a * texColor.a;
    return litColor;
}
