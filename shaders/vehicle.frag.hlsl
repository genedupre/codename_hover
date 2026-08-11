struct FragmentInput
{
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
    float3 color : TEXCOORD1;
};

float4 main(FragmentInput input) : SV_Target0
{
    const float3 directionToLight = normalize(float3(-0.45f, 1.0f, -0.30f));
    const float diffuse = saturate(dot(normalize(input.normal), directionToLight));
    const float lighting = 0.32f + 0.78f * diffuse;
    return float4(input.color * lighting, 1.0f);
}
