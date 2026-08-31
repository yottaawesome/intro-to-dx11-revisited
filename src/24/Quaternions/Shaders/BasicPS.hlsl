#include "BasicCommon.hlsli"

float4 main(VertexOut pin) : SV_Target
{
    pin.NormalW = normalize(pin.NormalW);

    float3 toEye = gEyePosW - pin.PosW;
    float distanceToEye = length(toEye);
    toEye /= distanceToEye;

    float4 textureColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (gUseTexture)
    {
        textureColor = gDiffuseMap.Sample(samAnisotropic, pin.Tex);

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

            ambient += lightAmbient;
            diffuse += lightDiffuse;
            specular += lightSpecular;
        }

        litColor = textureColor * (ambient + diffuse) + specular;
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
