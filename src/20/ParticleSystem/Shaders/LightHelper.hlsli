struct DirectionalLight
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float3 Direction;
    float Pad;
};

struct PointLight
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float3 Position;
    float Range;
    float3 Att;
    float Pad;
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
    float3 Att;
    float Pad;
};

struct Material
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float4 Reflect;
};

void ComputeDirectionalLight(
    Material mat,
    DirectionalLight light,
    float3 normal,
    float3 toEye,
    out float4 ambient,
    out float4 diffuse,
    out float4 spec)
{
    ambient = mat.Ambient * light.Ambient;
    diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

    float3 lightVec = -light.Direction;
    float diffuseFactor = dot(lightVec, normal);

    [flatten]
    if (diffuseFactor > 0.0f)
    {
        float3 reflectedLight = reflect(-lightVec, normal);
        float specFactor = pow(max(dot(reflectedLight, toEye), 0.0f), mat.Specular.w);

        diffuse = diffuseFactor * mat.Diffuse * light.Diffuse;
        spec = specFactor * mat.Specular * light.Specular;
    }
}

void ComputePointLight(
    Material mat,
    PointLight light,
    float3 pos,
    float3 normal,
    float3 toEye,
    out float4 ambient,
    out float4 diffuse,
    out float4 spec)
{
    ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

    float3 lightVec = light.Position - pos;
    float distanceToLight = length(lightVec);
    if (distanceToLight > light.Range)
    {
        return;
    }

    lightVec /= distanceToLight;
    ambient = mat.Ambient * light.Ambient;

    float diffuseFactor = dot(lightVec, normal);
    [flatten]
    if (diffuseFactor > 0.0f)
    {
        float3 reflectedLight = reflect(-lightVec, normal);
        float specFactor = pow(max(dot(reflectedLight, toEye), 0.0f), mat.Specular.w);

        diffuse = diffuseFactor * mat.Diffuse * light.Diffuse;
        spec = specFactor * mat.Specular * light.Specular;
    }

    float attenuation = 1.0f / dot(
        light.Att,
        float3(1.0f, distanceToLight, distanceToLight * distanceToLight));
    diffuse *= attenuation;
    spec *= attenuation;
}

void ComputeSpotLight(
    Material mat,
    SpotLight light,
    float3 pos,
    float3 normal,
    float3 toEye,
    out float4 ambient,
    out float4 diffuse,
    out float4 spec)
{
    ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

    float3 lightVec = light.Position - pos;
    float distanceToLight = length(lightVec);
    if (distanceToLight > light.Range)
    {
        return;
    }

    lightVec /= distanceToLight;
    ambient = mat.Ambient * light.Ambient;

    float diffuseFactor = dot(lightVec, normal);
    [flatten]
    if (diffuseFactor > 0.0f)
    {
        float3 reflectedLight = reflect(-lightVec, normal);
        float specFactor = pow(max(dot(reflectedLight, toEye), 0.0f), mat.Specular.w);

        diffuse = diffuseFactor * mat.Diffuse * light.Diffuse;
        spec = specFactor * mat.Specular * light.Specular;
    }

    float spot = pow(max(dot(-lightVec, light.Direction), 0.0f), light.Spot);
    float attenuation = spot / dot(
        light.Att,
        float3(1.0f, distanceToLight, distanceToLight * distanceToLight));

    ambient *= spot;
    diffuse *= attenuation;
    spec *= attenuation;
}
