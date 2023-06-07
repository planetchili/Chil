struct Output
{
	float2 uv : TEXCOORD;
	float4 position : SV_Position;
};

Output main(float3 pos : POSITION, float2 uv : TEXCOORD)
{
	Output vertexOut;
	vertexOut.position = float4(pos, 1.f);
	vertexOut.uv = uv;

	return vertexOut;
}