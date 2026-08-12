struct FragmentInput
{
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
    float4 color : TEXCOORD1;
};

float4 main(FragmentInput input) : SV_Target0
{
    // The pulse is deliberately unlit so it reads like emitted energy even with
    // the project's otherwise simple directional lighting.
    return input.color;
}
