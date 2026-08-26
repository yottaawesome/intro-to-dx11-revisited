#include "TerrainCommon.hlsli"

float4 main(TerrainDomainOut pin) : SV_Target
{
    float2 leftTex = pin.Tex + float2(-gTexelCellSpaceU, 0.0f);
    float2 rightTex = pin.Tex + float2(gTexelCellSpaceU, 0.0f);
    float2 bottomTex = pin.Tex + float2(0.0f, gTexelCellSpaceV);
    float2 topTex = pin.Tex + float2(0.0f, -gTexelCellSpaceV);

    float leftY = gHeightMap.SampleLevel(samHeightmap, leftTex, 0).r;
    float rightY = gHeightMap.SampleLevel(samHeightmap, rightTex, 0).r;
    float bottomY = gHeightMap.SampleLevel(samHeightmap, bottomTex, 0).r;
    float topY = gHeightMap.SampleLevel(samHeightmap, topTex, 0).r;

    float3 tangent = normalize(
        float3(2.0f * gWorldCellSpace, rightY - leftY, 0.0f));
    float3 bitangent = normalize(
        float3(0.0f, bottomY - topY, -2.0f * gWorldCellSpace));
    float3 normalW = cross(tangent, bitangent);

    float3 toEye = gEyePosW - pin.PosW;
    float distToEye = length(toEye);
    toEye /= distToEye;

    float4 layer0 = gLayerMapArray.Sample(
        samLinear, float3(pin.TiledTex, 0.0f));
    float4 layer1 = gLayerMapArray.Sample(
        samLinear, float3(pin.TiledTex, 1.0f));
    float4 layer2 = gLayerMapArray.Sample(
        samLinear, float3(pin.TiledTex, 2.0f));
    float4 layer3 = gLayerMapArray.Sample(
        samLinear, float3(pin.TiledTex, 3.0f));
    float4 layer4 = gLayerMapArray.Sample(
        samLinear, float3(pin.TiledTex, 4.0f));
    float4 blend = gBlendMap.Sample(samLinear, pin.Tex);

    float4 texColor = layer0;
    texColor = lerp(texColor, layer1, blend.r);
    texColor = lerp(texColor, layer2, blend.g);
    texColor = lerp(texColor, layer3, blend.b);
    texColor = lerp(texColor, layer4, blend.a);

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
                normalW,
                toEye,
                lightAmbient,
                lightDiffuse,
                lightSpec);
            ambient += lightAmbient;
            diffuse += lightDiffuse;
            spec += lightSpec;
        }

        litColor = texColor * (ambient + diffuse) + spec;
    }

    if (gFogEnabled)
    {
        float fogLerp = saturate((distToEye - gFogStart) / gFogRange);
        litColor = lerp(litColor, gFogColor, fogLerp);
    }

    return litColor;
}
