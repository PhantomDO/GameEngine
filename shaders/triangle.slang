// Premier triangle (M1.3), d'après le Basic Triangle de Donut-Samples : aucun vertex buffer, les
// trois sommets sont écrits dans le shader et choisis par leur numéro.

static const float2 Positions[3] = {
    float2(-0.5, -0.5),
    float2(0.0, 0.5),
    float2(0.5, -0.5),
};

static const float3 Colors[3] = {
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0),
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

[shader("vertex")]
VertexOutput vertexMain(uint vertexId : SV_VertexID)
{
    VertexOutput output;
    output.position = float4(Positions[vertexId], 0.0, 1.0);
    output.color = Colors[vertexId];
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}
