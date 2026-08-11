cbuffer CameraData : register(b0, space1)
{
    column_major float4x4 viewProjection;
    column_major float4x4 model;
};

struct VertexInput
{
    float3 position : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 color : TEXCOORD2;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
    float3 color : TEXCOORD1;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    const float4 worldPosition = mul(model, float4(input.position, 1.0f));
    output.position = mul(viewProjection, worldPosition);
    output.normal = mul((float3x3)model, input.normal);
    output.color = input.color;
    return output;
}
