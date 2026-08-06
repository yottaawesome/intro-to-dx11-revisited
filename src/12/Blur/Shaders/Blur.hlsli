static const float gWeights[11] =
{
    0.05f, 0.05f, 0.1f, 0.1f, 0.1f, 0.2f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f,
};

static const int gBlurRadius = 5;

Texture2D gInput;
RWTexture2D<float4> gOutput;

static const int N = 256;
static const int CacheSize = N + 2 * gBlurRadius;
groupshared float4 gCache[CacheSize];
