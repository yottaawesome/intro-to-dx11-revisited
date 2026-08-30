#include "BasicCommon.hlsli"

Texture2D gDiffuseMap : register(t0);
TextureCube gCubeMap : register(t1);
Texture2D gShadowMap : register(t2);
Texture2D gSsaoMap : register(t3);

SamplerState samLinear : register(s0);
SamplerComparisonState samShadow : register(s1);

float4 main(VertexOut pin) : SV_Target
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

    float4 litColor = textureColor;
    if (gLightCount > 0)
    {
        float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float4 specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

        float3 shadow = float3(1.0f, 1.0f, 1.0f);
        shadow[0] = CalcShadowFactor(samShadow, gShadowMap, pin.ShadowPosH);

        pin.SsaoPosH /= pin.SsaoPosH.w;
        float ambientAccess =
            gSsaoMap.SampleLevel(samLinear, pin.SsaoPosH.xy, 0.0f).r;

        [unroll(3)]
        for (uint i = 0; i < gLightCount; ++i)
        {
            float4 lightAmbient;
            float4 lightDiffuse;
            float4 lightSpecular;
            ComputeDirectionalLight(
                gMaterial,
                gDirLights[i],
                pin.NormalW,
                toEye,
                lightAmbient,
                lightDiffuse,
                lightSpecular);

            ambient += ambientAccess * lightAmbient;
            diffuse += shadow[i] * lightDiffuse;
            specular += shadow[i] * lightSpecular;
        }

        litColor = textureColor * (ambient + diffuse) + specular;

        if (gReflectionEnabled)
        {
            float3 reflectionVector = reflect(-toEye, pin.NormalW);
            float4 reflectionColor =
                gCubeMap.Sample(samLinear, reflectionVector);
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
