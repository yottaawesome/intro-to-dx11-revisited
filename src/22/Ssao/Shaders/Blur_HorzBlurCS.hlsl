#include "Blur.hlsli"

[numthreads(N, 1, 1)]
void main(
    int3 groupThreadID : SV_GroupThreadID,
    int3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width;
    uint height;
    gInput.GetDimensions(width, height);

    if (groupThreadID.x < gBlurRadius)
    {
        int x = max(dispatchThreadID.x - gBlurRadius, 0);
        gCache[groupThreadID.x] =
            gInput[int2(x, dispatchThreadID.y)];
    }

    if (groupThreadID.x >= N - gBlurRadius)
    {
        int x = min(
            dispatchThreadID.x + gBlurRadius,
            (int)width - 1);
        gCache[groupThreadID.x + 2 * gBlurRadius] =
            gInput[int2(x, dispatchThreadID.y)];
    }

    gCache[groupThreadID.x + gBlurRadius] =
        gInput[min(dispatchThreadID.xy, int2(width, height) - 1)];

    GroupMemoryBarrierWithGroupSync();

    float4 blurColor = float4(0.0f, 0.0f, 0.0f, 0.0f);

    [unroll]
    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        int cacheIndex = groupThreadID.x + gBlurRadius + i;
        blurColor +=
            gWeights[i + gBlurRadius] * gCache[cacheIndex];
    }

    gOutput[dispatchThreadID.xy] = blurColor;
}
