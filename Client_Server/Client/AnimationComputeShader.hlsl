
struct meshInfo {
    uint vertexCount;
    uint BonePerVertex;
};

struct bufferInfo
{
    int bHasNormal;
    int bHasTangent;
    int bHasBiTangnet;
};

// Skinning Range
ConstantBuffer<meshInfo> g_MeshInfo : register(b0, space0);        
StructuredBuffer<float4x4> g_OffsetMatrix : register(t0, space0);
StructuredBuffer<uint> g_BoneIndices : register(t1, space0);
StructuredBuffer<float> g_BoneWeight : register(t2, space0);
StructuredBuffer<uint> g_AnimationMatrixIndex : register(t3, space0);
StructuredBuffer<float4x4> g_AnimationMatrix : register(t4, space0);

ConstantBuffer<bufferInfo> g_BufferInfo : register(b1, space0);
StructuredBuffer<float3> g_InsertVertexBuffer : register(t5, space0);
StructuredBuffer<float3> g_InsertNormalBuffer : register(t6, space0);
StructuredBuffer<float3> g_InsertTangentBuffer : register(t7, space0);
StructuredBuffer<float3> g_InsertBiTangentBuffer : register(t8, space0);
// RW
RWStructuredBuffer<float3> g_OutputVertexBuffer : register(u0, space0);
RWStructuredBuffer<float3> g_OutputNormalBuffer : register(u1, space0);
RWStructuredBuffer<float3> g_OutputTangentBuffer : register(u2, space0);
RWStructuredBuffer<float3> g_OutputBiTangentBuffer : register(u3, space0);

float3x3 InverseFloat3x3(float3x3 m)
{
    float a00 = m[0][0], a01 = m[0][1], a02 = m[0][2];
    float a10 = m[1][0], a11 = m[1][1], a12 = m[1][2];
    float a20 = m[2][0], a21 = m[2][1], a22 = m[2][2];

    float b01 = a22 * a11 - a12 * a21;
    float b11 = -a22 * a10 + a12 * a20;
    float b21 = a21 * a10 - a11 * a20;

    float det = a00 * b01 + a01 * b11 + a02 * b21;

    // 너무 작은 행렬이면 그냥 단위행렬 반환
    if (abs(det) < 1e-5)
        return float3x3(1, 0, 0, 0, 1, 0, 0, 0, 1);

    float invDet = 1.0 / det;

    float3x3 inv;
    inv[0][0] = b01 * invDet;
    inv[0][1] = (-a22 * a01 + a02 * a21) * invDet;
    inv[0][2] = (a12 * a01 - a02 * a11) * invDet;
    inv[1][0] = b11 * invDet;
    inv[1][1] = (a22 * a00 - a02 * a20) * invDet;
    inv[1][2] = (-a12 * a00 + a02 * a10) * invDet;
    inv[2][0] = b21 * invDet;
    inv[2][1] = (-a21 * a00 + a01 * a20) * invDet;
    inv[2][2] = (a11 * a00 - a01 * a10) * invDet;

    return inv;
}


[numthreads(32, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    if (DTid.x < g_MeshInfo.vertexCount)
    {
        uint BonePerVertex = g_MeshInfo.BonePerVertex;
        float4x4 weightMatrix = (float4x4) 0.0f;
        if (0 != g_BufferInfo.bHasTangent)  // exist TBN coordinate
        {
            for (int i = 0; i < BonePerVertex; ++i)
            {
                uint boneIndex = g_BoneIndices[(DTid.x * BonePerVertex) + i];
                weightMatrix += g_BoneWeight[(DTid.x * BonePerVertex) + i] * mul(g_OffsetMatrix[boneIndex],
            g_AnimationMatrix[g_AnimationMatrixIndex[boneIndex]]);
            }
            float3x3 normalMatrix = transpose(InverseFloat3x3((float3x3) weightMatrix));

            g_OutputVertexBuffer[DTid.x] = mul(float4(g_InsertVertexBuffer[DTid.x], 1.0), weightMatrix).xyz;
            g_OutputNormalBuffer[DTid.x] = mul(g_InsertNormalBuffer[DTid.x], normalMatrix);
            g_OutputTangentBuffer[DTid.x] = mul(g_InsertTangentBuffer[DTid.x], normalMatrix);
            g_OutputBiTangentBuffer[DTid.x] = mul(g_InsertBiTangentBuffer[DTid.x], normalMatrix);
        }
        else if (0 != g_BufferInfo.bHasNormal)  // exist NormaBuffer
        {
            for (int i = 0; i < BonePerVertex; ++i)
            {
                uint boneIndex = g_BoneIndices[(DTid.x * BonePerVertex) + i];
                weightMatrix += g_BoneWeight[(DTid.x * BonePerVertex) + i] * mul(g_OffsetMatrix[boneIndex],
            g_AnimationMatrix[g_AnimationMatrixIndex[boneIndex]]);
            }
            float3x3 normalMatrix = transpose(InverseFloat3x3((float3x3) weightMatrix));

            g_OutputVertexBuffer[DTid.x] = mul(float4(g_InsertVertexBuffer[DTid.x], 1.0), weightMatrix).xyz;
            //g_OutputNormalBuffer[DTid.x] = mul(g_InsertNormalBuffer[DTid.x], (float3x3)weightMatrix);
            g_OutputNormalBuffer[DTid.x] = mul(g_InsertNormalBuffer[DTid.x], normalMatrix);
        }
        else                                    // only Vertex
        {
            for (int i = 0; i < BonePerVertex; ++i)
            {
                uint boneIndex = g_BoneIndices[(DTid.x * BonePerVertex) + i];
                weightMatrix += g_BoneWeight[(DTid.x * BonePerVertex) + i] * mul(g_OffsetMatrix[boneIndex],
            g_AnimationMatrix[g_AnimationMatrixIndex[boneIndex]]);
            }
            g_OutputVertexBuffer[DTid.x] = mul(float4(g_InsertVertexBuffer[DTid.x], 1.0), weightMatrix).xyz;
        }
    }
}

/*
for (int i = 0; i < BonePerVertex; ++i)
            {
                uint boneIndex = g_BoneIndices[(DTid.x * BonePerVertex) + i];
                float boneWeight = g_BoneWeight[(DTid.x * BonePerVertex) + i];
            
                float4 input = float4(g_InsertVertexBuffer[DTid.x], 1.0f);
                float4x4 boneMatrix = mul(g_OffsetMatrix[boneIndex],
            g_AnimationMatrix[g_AnimationMatrixIndex[boneIndex]]);
                weightPos += boneWeight * mul(input, boneMatrix).xyz;
            }
            g_OutputVertexBuffer[DTid.x] = weightPos;
*/