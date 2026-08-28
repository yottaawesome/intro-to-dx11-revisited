#include "DisplacementMapCommon.hlsli"

Texture2D gDiffuseMap : register(t0);
Texture2D gNormalMap : register(t1);
TextureCube gCubeMap : register(t2);
Texture2D gShadowMap : register(t3);

SamplerState samLinear : register(s0);
SamplerComparisonState samShadow : register(s1);

float4 main(DomainOut pin) : SV_Target
{
    pin.NormalW = normalize(pin.NormalW);

    float3 toEye = gEyePosW - pin.PosW;
    float distanceToEye = length(toEye);
    toEye /= distanceToEye;

    float4 textureColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (gUseTexture)
    {
        textureColor = gDiffuseMap.Sample(samLinear, pin.Tex);

        if (gAlphaClip)
        {
            clip(textureColor.a - 0.1f);
        }
    }

    float3 normalMapSample = gNormalMap.Sample(samLinear, pin.Tex).rgb;
    float3 bumpedNormalW =
        NormalSampleToWorldSpace(normalMapSample, pin.NormalW, pin.TangentW);

    float4 litColor = textureColor;
    if (gLightCount > 0)
    {
        float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float4 specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

        float3 shadow = float3(1.0f, 1.0f, 1.0f);
        shadow[0] = CalcShadowFactor(samShadow, gShadowMap, pin.ShadowPosH);

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
            diffuse += shadow[i] * lightDiffuse;
            specular += shadow[i] * lightSpecular;
        }

        litColor = textureColor * (ambient + diffuse) + specular;

        if (gReflectionEnabled)
        {
            float3 reflectionVector = reflect(-toEye, bumpedNormalW);
            float4 reflectionColor = gCubeMap.Sample(samLinear, reflectionVector);
            litColor += gMaterial.Reflect * reflectionColor;
        }
    }

    if (gFogEnabled)
    {
        float fogAmount =
            saturate((distanceToEye - gFogStart) / gFogRange);
        litColor = lerp(litColor, gFogColor, fogAmount);
    }

    litColor.a = gMaterial.Diffuse.a * textureColor.a;
    return litColor;
}
