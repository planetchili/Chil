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

Output main(
    float3 pos : POSITION,
    float2 uv : TEXCOORD,
    float2 tl : TRANSLATION,
    float2 scale : SCALE,
    float rot : ROTATION,
    uint atlasIndex : ATLASINDEX)
{
    // generate the per-sprite (object) transform matrix
    float s, c;
    sincos(rot, s, c);
    const matrix objTransform = {
        scale.x * c, -scale.y * s, 0, tl.x,
        scale.x * s, scale.y * c, 0, tl.y,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    
    // concatenate object and camera matrices
    const matrix transform = mul(objTransform, cam.transform);
    
    // generate output to pixel shader
	Output vertexOut;
    vertexOut.position = mul(float4(pos, 1.f), transform);
	vertexOut.uv = uv;
    vertexOut.atlasIndex = atlasIndex;

	return vertexOut;
}