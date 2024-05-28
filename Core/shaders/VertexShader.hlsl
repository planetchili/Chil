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

struct Frame
{
    float2 pivotPixelCoords;
    float2 frameTexPos;
    float2 frameTexDims;
	uint destPixelDimsPacked;
	uint atlasIndex;    
};
StructuredBuffer<Frame> frames : register(t0);

Output main(
    // Per-vertex data
    float2 unitPos : POSITION,
    // Instance data
    float2 tl : TRANSLATION,
    float rot : ROTATION,
    float2 scale : SCALE,
    uint frameIndex : FRAMEINDEX)
{
    // load the frame data
    const Frame fd = frames[frameIndex];
    
    // generate the per-sprite (object) transform matrix
    float s, c;
    sincos(rot, s, c);
    const matrix objTransform = {
        scale.x * c, scale.x * s, 0, 0,
        -scale.y * s, scale.y * c, 0, 0,
        0, 0, 1, 0,
        scale.y * fd.pivotPixelCoords.y * s - c * scale.x * fd.pivotPixelCoords.x + tl.x,
            -scale.y * fd.pivotPixelCoords.y * c - s * scale.x * fd.pivotPixelCoords.x + tl.y, 0, 1
    };
    
    // unpack dest pixel dims
    const float2 destPixelDims = float2(fd.destPixelDimsPacked & 0xFFFF, fd.destPixelDimsPacked >> 16);
    
    // concatenate object and camera matrices
    const matrix transform = mul(objTransform, cam.transform);
    
    // recover output position
    const float2 pos = unitPos * destPixelDims;
    
    // recover selector for which texcoord corresponds to this vertex
    const float2 texAxes = float2(unitPos.x + 0.5f, 0.5f - unitPos.y);
    
    // generate output to pixel shader
	Output vertexOut;
    vertexOut.position = mul(float4(pos, 0.f, 1.f), transform);
    vertexOut.uv = fd.frameTexPos + fd.frameTexDims * texAxes;
    vertexOut.atlasIndex = fd.atlasIndex;

	return vertexOut;
}