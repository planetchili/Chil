#include "Util.hlsli"

struct Output
{
    float2 uv : TEXCOORD;
    nointerpolation uint atlasIndex : ATLASINDEX;
    float4 position : SV_Position;
};

struct LayerConstants
{
    matrix cameraTransform;
    float tileSizeTc;
    float tileSizeWorld;
};
ConstantBuffer<LayerConstants> lc : register(b0);

struct BlockConstants
{
    float2 worldPos;
};
ConstantBuffer<BlockConstants> bc : register(b1);


Output main(
    // Per-vertex data
    float2 basePos : POSITION,
    // Instance data (static)
    float2 gridPos : GRIDPOS,
    // Instance data (per block)
    uint atlasIdxTileIdPacked : ATLASTILE)
{
    // unpack the atlas index and tile id
    const uint2 atlasIdxTileIdUnpacked = UnpackUint16(atlasIdxTileIdPacked);
    const uint atlasIdx = atlasIdxTileIdUnpacked.x;
    const float2 tileGridPos = (float2) UnpackUint8(atlasIdxTileIdUnpacked.y);
    
    // generate texture coordinates for the tile vertex
    const float2 tc = (basePos + tileGridPos) * lc.tileSizeTc.xx;
        
    // generate world coordinates for the tile vertex
    const float2 wc = (basePos + tileGridPos) * lc.tileSizeWorld.xx + bc.worldPos;
    
    // generate output to pixel shader
    Output vertexOut;
    vertexOut.position = mul(float4(wc, 0.f, 1.f), lc.cameraTransform);
    vertexOut.uv = tc;
    vertexOut.atlasIndex = atlasIdx;

    return vertexOut;
}