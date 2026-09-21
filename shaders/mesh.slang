// Un mesh indexé, coloré par sommet (M2.1). Les constantes suivent l'ADR-0013 : space0 pour ce qui
// change une fois par frame.

struct SceneConstants
{
    float4x4 viewProjection;
    float4x4 model;
};

ConstantBuffer<SceneConstants> scene : register(b0, space0);

struct VertexInput
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    // mul(matrice, vecteur) : les matrices arrivent de GLM rangées par colonnes, et le shader est
    // compilé avec -matrix-layout-column-major (shaders/CMakeLists.txt).
    VertexOutput output;
    output.position = mul(scene.viewProjection, mul(scene.model, float4(input.position, 1.0)));
    output.color = input.color;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}
