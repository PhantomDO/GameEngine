// Le skinning en compute (ADR-0022) : chaque sommet est déplacé par la moyenne pondérée des
// matrices de ses quatre os, et écrit au format de la passe des meshes, qui le dessine ensuite sans
// rien savoir de l'animation. Les buffers sont lus et écrits octet par octet (ByteAddressBuffer) :
// leur disposition est exactement celle de SkinnedVertex et de MeshVertex en C++, sans les
// alignements qu'imposerait un StructuredBuffer d'un float3 sous Vulkan.

static const uint SourceStride = 56; // SkinnedVertex : position, couleur, uv, os, poids
static const uint SkinnedStride = 32; // MeshVertex : position, couleur, uv

ByteAddressBuffer sourceVertices : register(t0);
StructuredBuffer<float4x4> jointMatrices : register(t1);
RWByteAddressBuffer skinnedVertices : register(u0);

[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 thread : SV_DispatchThreadID)
{
    // Le dernier groupe déborde : sa taille ne divise pas le nombre de sommets.
    uint skinnedBytes;
    skinnedVertices.GetDimensions(skinnedBytes);
    if (thread.x >= skinnedBytes / SkinnedStride)
    {
        return;
    }

    const uint source = thread.x * SourceStride;
    const float3 position = asfloat(sourceVertices.Load3(source));
    const uint3 colorBits = sourceVertices.Load3(source + 12);
    const uint2 uvBits = sourceVertices.Load2(source + 24);
    // Quatre indices de 16 bits, rangés deux par mot.
    const uint2 packedJoints = sourceVertices.Load2(source + 32);
    const uint4 joints = uint4(packedJoints.x & 0xFFFF, packedJoints.x >> 16,
                               packedJoints.y & 0xFFFF, packedJoints.y >> 16);
    const float4 weights = asfloat(sourceVertices.Load4(source + 40));

    const float4x4 skin = weights.x * jointMatrices[joints.x] + weights.y * jointMatrices[joints.y] +
                          weights.z * jointMatrices[joints.z] + weights.w * jointMatrices[joints.w];
    const float3 skinned = mul(skin, float4(position, 1.0)).xyz;

    const uint target = thread.x * SkinnedStride;
    skinnedVertices.Store3(target, asuint(skinned));
    skinnedVertices.Store3(target + 12, colorBits);
    skinnedVertices.Store2(target + 24, uvBits);
}
