struct Output
{
	float2 uv : TEXCOORD;
    nointerpolation uint atlasIndex : ATLASINDEX;
    float4 position : SV_Position;
};

struct Camera
{
    matrix transform;
};
ConstantBuffer<Camera> cam : register(b0);

Output main(float3 pos : POSITION, float2 uv : TEXCOORD, uint atlasIndex : ATLASINDEX)
{
	Output vertexOut;
    vertexOut.position = mul(float4(pos, 1.f), cam.transform);
	vertexOut.uv = uv;
    vertexOut.atlasIndex = atlasIndex;

	return vertexOut;
}