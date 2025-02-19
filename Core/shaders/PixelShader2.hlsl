Texture2D textures[] : register(t0);
SamplerState samp : register(s0);

float4 main(float2 uv : TEXCOORD, float4 tint : TINT, uint atlasIndex : ATLASINDEX) : SV_TARGET
{
    const float4 texel = textures[atlasIndex].Sample(samp, uv);    
    if (texel.a == 0.f) discard;
    return texel * tint;
}