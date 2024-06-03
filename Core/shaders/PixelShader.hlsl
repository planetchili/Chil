Texture2D textures[] : register(t1);
SamplerState samp : register(s0);

// NonUniformResourceIndex
float4 main(float2 uv : TEXCOORD, uint atlasIndex : ATLASINDEX) : SV_TARGET
{
    const float4 pixel = textures[atlasIndex].Sample(samp, uv);
	return pixel;
}