struct Output
{
	float2 uv : TEXCOORD;
    nointerpolation uint atlasIndex : ATLASINDEX;
    float4 position : SV_Position;
};

Output main(float3 pos : POSITION, float2 uv : TEXCOORD, uint atlasIndex : ATLASINDEX)
{
	Output vertexOut;
	vertexOut.position = float4(pos, 1.f);
	vertexOut.uv = uv;
    vertexOut.atlasIndex = atlasIndex;

	return vertexOut;
}