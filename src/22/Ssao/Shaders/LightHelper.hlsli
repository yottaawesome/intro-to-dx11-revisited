struct DirectionalLight
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float3 Direction;
    float Padding;
};

struct PointLight
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float3 Position;
    float Range;
    float3 Attenuation;
    float Padding;
};

struct SpotLight
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float3 Position;
    float Range;
    float3 Direction;
    float Spot;
    float3 Attenuation;
    float Padding;
};

struct Material
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float4 Reflect;
};

void ComputeDirectionalLight(
    Material material,
    DirectionalLight light,
    float3 normal,
    float3 toEye,
    out float4 ambient,
    out float4 diffuse,
    out float4 specular)
{
    ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

    float3 lightVector = -light.Direction;
    ambient = material.Ambient * light.Ambient;

    float diffuseFactor = dot(lightVector, normal);

    [flatten]
    if (diffuseFactor > 0.0f)
    {
        float3 reflectedLight = reflect(-lightVector, normal);
        float specularFactor =
            pow(max(dot(reflectedLight, toEye), 0.0f), material.Specular.w);

        diffuse = diffuseFactor * material.Diffuse * light.Diffuse;
        specular = specularFactor * material.Specular * light.Specular;
    }
}

void ComputePointLight(
    Material material,
    PointLight light,
    float3 position,
    float3 normal,
    float3 toEye,
    out float4 ambient,
    out float4 diffuse,
    out float4 specular)
{
    ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

    float3 lightVector = light.Position - position;
    float distanceToLight = length(lightVector);

    if (distanceToLight > light.Range)
    {
        return;
    }

    lightVector /= distanceToLight;
    ambient = material.Ambient * light.Ambient;

    float diffuseFactor = dot(lightVector, normal);

    [flatten]
    if (diffuseFactor > 0.0f)
    {
        float3 reflectedLight = reflect(-lightVector, normal);
        float specularFactor =
            pow(max(dot(reflectedLight, toEye), 0.0f), material.Specular.w);

        diffuse = diffuseFactor * material.Diffuse * light.Diffuse;
        specular = specularFactor * material.Specular * light.Specular;
    }

    float attenuation = 1.0f / dot(
        light.Attenuation,
        float3(1.0f, distanceToLight, distanceToLight * distanceToLight));

    diffuse *= attenuation;
    specular *= attenuation;
}

void ComputeSpotLight(
    Material material,
    SpotLight light,
    float3 position,
    float3 normal,
    float3 toEye,
    out float4 ambient,
    out float4 diffuse,
    out float4 specular)
{
    ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

    float3 lightVector = light.Position - position;
    float distanceToLight = length(lightVector);

    if (distanceToLight > light.Range)
    {
        return;
    }

    lightVector /= distanceToLight;
    ambient = material.Ambient * light.Ambient;

    float diffuseFactor = dot(lightVector, normal);

    [flatten]
    if (diffuseFactor > 0.0f)
    {
        float3 reflectedLight = reflect(-lightVector, normal);
        float specularFactor =
            pow(max(dot(reflectedLight, toEye), 0.0f), material.Specular.w);

        diffuse = diffuseFactor * material.Diffuse * light.Diffuse;
        specular = specularFactor * material.Specular * light.Specular;
    }

    float spotFactor =
        pow(max(dot(-lightVector, light.Direction), 0.0f), light.Spot);
    float attenuation = spotFactor / dot(
        light.Attenuation,
        float3(1.0f, distanceToLight, distanceToLight * distanceToLight));

    ambient *= spotFactor;
    diffuse *= attenuation;
    specular *= attenuation;
}

float3 NormalSampleToWorldSpace(
    float3 normalMapSample,
    float3 unitNormalW,
    float3 tangentW)
{
    float3 normalT = 2.0f * normalMapSample - 1.0f;

    float3 normalW = unitNormalW;
    float3 tangent = normalize(tangentW - dot(tangentW, normalW) * normalW);
    float3 bitangent = cross(normalW, tangent);
    float3x3 tangentToWorld = float3x3(tangent, bitangent, normalW);

    return mul(normalT, tangentToWorld);
}

float CalcShadowFactor(
    SamplerComparisonState shadowSampler,
    Texture2D shadowMap,
    float4 shadowPositionH)
{
    shadowPositionH.xyz /= shadowPositionH.w;

    static const float shadowMapSize = 2048.0f;
    static const float texelSize = 1.0f / shadowMapSize;
    static const float2 offsets[9] =
    {
        float2(-texelSize, -texelSize),
        float2(0.0f, -texelSize),
        float2(texelSize, -texelSize),
        float2(-texelSize, 0.0f),
        float2(0.0f, 0.0f),
        float2(texelSize, 0.0f),
        float2(-texelSize, texelSize),
        float2(0.0f, texelSize),
        float2(texelSize, texelSize)
    };

    float percentLit = 0.0f;

    [unroll]
    for (uint i = 0; i < 9; ++i)
    {
        percentLit += shadowMap.SampleCmpLevelZero(
            shadowSampler,
            shadowPositionH.xy + offsets[i],
            shadowPositionH.z);
    }

    return percentLit / 9.0f;
}
