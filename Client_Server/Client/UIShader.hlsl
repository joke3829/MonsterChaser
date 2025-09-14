
struct VS_TEXTURED_INPUT
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};


struct VS_TEXTURED_OUTPUT
{
    float4 posW : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};


struct CameraInfo
{
    matrix orthoProj;
};

struct ObjectConstant
{
    matrix world;
    matrix uvTransform;
    int bHasTexture;
};

ConstantBuffer<CameraInfo> g_Camera : register(b0, space0);
ConstantBuffer<ObjectConstant> g_ObjectInfo : register(b1, space0);
Texture2D g_txt : register(t0, space0);
SamplerState g_Sampler : register(s0, space0);

VS_TEXTURED_OUTPUT VSMain(VS_TEXTURED_INPUT input)
{
    VS_TEXTURED_OUTPUT output;
    output.posW = mul(mul(float4(input.position, 1.0f), g_ObjectInfo.world), g_Camera.orthoProj);
    output.color = input.color;
    output.uv = mul(float4(input.uv, 0.0f, 1.0f), g_ObjectInfo.uvTransform).xy;;
    return output;
}

float4 PSMain(VS_TEXTURED_OUTPUT input) : SV_TARGET
{
    float4 OutputColor;
    if(0 != g_ObjectInfo.bHasTexture)
        OutputColor = g_txt.SampleLevel(g_Sampler, input.uv, 0);
    else
    {
        OutputColor = input.color;
    }
    return OutputColor;
}

/*float4 VSMain(uint nVertexID : SV_VertexID) : SV_POSITION
{
    float4 output;
    
    if (nVertexID == 0)
        output = float4(0.0, 0.5, 0.5, 1.0);
    else if (nVertexID == 1)
        output = float4(0.5, -0.5, 0.5, 1.0);
    else if (nVertexID == 2)
        output = float4(-0.5, -0.5, 0.5, 1.0);
    
    return output;
}

float4 PSMain(float4 input : SV_POSITION) : SV_TARGET
{
    return float4(1.0, 1.0, 0.0, 1.0);
}*/