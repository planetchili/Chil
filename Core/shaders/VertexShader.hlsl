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
    // pivot is expected to be in pixel units, with 0,0 at center of sprite frame and +ve up/right
    float2 pivot : PIVOT,
    float2 frameTexPos : TEXPOS,
    float2 frameTexDims : TEXDIMS,
    uint2 destPixelDims : DESTDIMS,
    uint atlasIndex : ATLASINDEX)
{
    // generate the per-sprite (object) transform matrix
    float s, c;
    sincos(rot, s, c);
    const matrix objTransform = {
        scale.x * c, scale.x * s, 0, 0,
        -scale.y * s, scale.y * c, 0, 0,
        0, 0, 1, 0,
        scale.y * pivot.y * s - c * scale.x * pivot.x + tl.x, -scale.y * pivot.y * c - s * scale.x * pivot.x + tl.y, 0, 1
    };
    
    // concatenate object and camera matrices
    const matrix transform = mul(objTransform, cam.transform);
    
    // recover output position
    const float2 pos = unitPos * destPixelDims;
    
    // recover selector for which texcoord corresponds to this vertex
    const float2 texAxes = float2(unitPos.x + 0.5f, 0.5f - unitPos.y);
    
    // generate output to pixel shader
	Output vertexOut;
    vertexOut.position = mul(float4(pos, 0.f, 1.f), transform);
    vertexOut.uv = frameTexPos + frameTexDims * texAxes;
    vertexOut.atlasIndex = atlasIndex;

	return vertexOut;
}