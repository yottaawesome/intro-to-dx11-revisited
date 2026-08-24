#include "DisplacementMapCommon.hlsli"

Texture2D gDiffuseMap : register(t0);
Texture2D gNormalMap : register(t1);
TextureCube gCubeMap : register(t2);

SamplerState samLinear : register(s0);

float4 main(DomainOut pin) : SV_Target
{
    pin.NormalW = normalize(pin.NormalW);

    float3 toEye = gEyePosW - pin.PosW;
    float distToEye = length(toEye);
    toEye /= distToEye;

    float4 texColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (gUseTexture)
    {
        texColor = gDiffuseMap.Sample(samLinear, pin.Tex);

        if (gAlphaClip)
        {
            clip(texColor.a - 0.1f);
        }
    }

    float3 normalMapSample = gNormalMap.Sample(samLinear, pin.Tex).rgb;
    float3 bumpedNormalW =
        NormalSampleToWorldSpace(normalMapSample, pin.NormalW, pin.TangentW);

    float4 litColor = texColor;
    if (gLightCount > 0)
    {
        float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float4 specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

        [unroll]
        for (uint i = 0; i < gLightCount; ++i)
        {
            float4 lightAmbient;
            float4 lightDiffuse;
            float4 lightSpecular;
            ComputeDirectionalLight(
                gMaterial,
                gDirLights[i],
                bumpedNormalW,
                toEye,
                lightAmbient,
                lightDiffuse,
                lightSpecular);

            ambient += lightAmbient;
            diffuse += lightDiffuse;
            specular += lightSpecular;
        }

        litColor = texColor * (ambient + diffuse) + specular;

        if (gReflectionEnabled)
        {
            float3 reflectionVector = reflect(-toEye, bumpedNormalW);
            float4 reflectionColor =
                gCubeMap.Sample(samLinear, reflectionVector);

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
