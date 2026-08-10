cbuffer CameraData : register(b0, space1)
{
    column_major float4x4 viewProjection;
};

struct VertexInput
{
    float3 position : TEXCOORD0;
    float3 color : TEXCOORD1;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color : TEXCOORD0;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.position = mul(viewProjection, float4(input.position, 1.0f));
    output.color = input.color;
    return output;
}
