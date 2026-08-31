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
