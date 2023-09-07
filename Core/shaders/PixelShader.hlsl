Texture2D textures[] : register(t0);
SamplerState samp : register(s0);

float4 main(float2 uv : TEXCOORD, uint atlasIndex : ATLASINDEX) : SV_TARGET
{
	return textures[atlasIndex].Sample(samp, uv);
}