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
    // Per-vertex data
    float2 unitPos : POSITION,
    // Instance data
    float2 tl : TRANSLATION,
    float rot : ROTATION,
    float2 scale : SCALE,
    float4 texRect : TEXRECT,
    uint2 destDims : DEST_DIMS,
    uint atlasIndex : ATLASINDEX)
{
    // generate the per-sprite (object) transform matrix
    float s, c;
    sincos(rot, s, c);
    const matrix objTransform = {
        scale.x * c, scale.x * s, 0, 0,
        -scale.y * s, scale.y * c, 0, 0,
        0, 0, 1, 0,
        tl.x, tl.y, 0, 1
    };
    
    // concatenate object and camera matrices
    const matrix transform = mul(objTransform, cam.transform);
    
    // geometry generate
    const float2 pos = unitPos * destDims;
    
    // recover texcoords
    float2 texTopLeft = texRect.xy;
    float2 texDims = float2(texRect.z - texRect.x, texRect.a - texRect.y);
    float2 texAxes = unitPos + float2(0.5, 0.5);
    
    // generate output to pixel shader
	Output vertexOut;
    vertexOut.position = mul(float4(pos, 0.f, 1.f), transform);
    vertexOut.uv = texTopLeft + texDims * texAxes;
    vertexOut.atlasIndex = atlasIndex;

	return vertexOut;
}