#include "Blur.hlsli"

[numthreads(1, N, 1)]
void main(
    int3 groupThreadID : SV_GroupThreadID,
    int3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width;
    uint height;
    gInput.GetDimensions(width, height);

    if (groupThreadID.y < gBlurRadius)
    {
        int y = max(dispatchThreadID.y - gBlurRadius, 0);
        gCache[groupThreadID.y] =
            gInput[int2(dispatchThreadID.x, y)];
    }

    if (groupThreadID.y >= N - gBlurRadius)
    {
        int y = min(
            dispatchThreadID.y + gBlurRadius,
            (int)height - 1);
        gCache[groupThreadID.y + 2 * gBlurRadius] =
            gInput[int2(dispatchThreadID.x, y)];
    }

    gCache[groupThreadID.y + gBlurRadius] =
        gInput[min(dispatchThreadID.xy, int2(width, height) - 1)];

    GroupMemoryBarrierWithGroupSync();

    float4 blurColor = float4(0.0f, 0.0f, 0.0f, 0.0f);

    [unroll]
    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        int cacheIndex = groupThreadID.y + gBlurRadius + i;
        blurColor +=
            gWeights[i + gBlurRadius] * gCache[cacheIndex];
    }

    gOutput[dispatchThreadID.xy] = blurColor;
}
