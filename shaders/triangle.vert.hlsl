struct VertexOutput
{
    float4 position : SV_Position;
    float3 color : TEXCOORD0;
};

VertexOutput main(uint vertexId : SV_VertexID)
{
    const float2 positions[3] = {
        float2(0.0f, -0.6f),
        float2(0.6f, 0.6f),
        float2(-0.6f, 0.6f),
    };

    const float3 colors[3] = {
        float3(1.0f, 0.15f, 0.15f),
        float3(0.15f, 1.0f, 0.15f),
        float3(0.15f, 0.35f, 1.0f),
    };

    VertexOutput output;
    output.position = float4(positions[vertexId], 0.0f, 1.0f);
    output.color = colors[vertexId];
    return output;
}
