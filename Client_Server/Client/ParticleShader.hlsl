
#define PARTICLE_EMITTER 0
#define PARTICLE_FLAME 1

struct ParticleVertex
{
    float3 position : POSITION;
    float3 direction : DIRECTION;
    float lifeTime : LIFETIME;
    uint particleType : TYPE;
};

struct BillBoardOutput1
{
    float3 position : POSITION;
};

struct BillBoardOutput2
{
    float2 uv : TEXCOORD;
};

struct BillBoardOutput3
{
    float4 color : COLOR;
};

struct CameraInfo
{
    matrix mtxViewProj;
    matrix mtxInverseViewProj;
    float3 cameraEye;
    float fElapsedTime;
    matrix mtxtarget;
    int bNormalMapping;
    int bReflection;
    int intMapNumber;
};

ConstantBuffer<CameraInfo> g_camera : register(b0, space0);

ParticleVertex VSMain(ParticleVertex input)
{
    return input;
}

[maxvertexcount(42)]
void GSOnePath(point ParticleVertex input[1], inout PointStream<ParticleVertex> outStream)
{
    ParticleVertex particle = input[0];
    particle.lifeTime -= g_camera.fElapsedTime;
    switch (particle.particleType)
    {
        case PARTICLE_EMITTER:{
                if (particle.lifeTime <= 0.0f)
                {
                    particle.lifeTime = 0.5f;
                    outStream.Append(particle);
                
                    ParticleVertex output;
                    output.direction = float3(0.0, 3.0, 0.0);
                    output.lifeTime = 4.0f;
                    output.particleType = PARTICLE_FLAME;
                    output.position = particle.position;
                    outStream.Append(output);
                }
                else
                {
                    outStream.Append(particle);
                }
            }
            break;
        case PARTICLE_FLAME:{
                if (particle.lifeTime > 0.0f)
                {
                    particle.position += (particle.direction * g_camera.fElapsedTime);
                    outStream.Append(particle);
                }
            }
            break;
    }
}

[maxvertexcount(252)]
void GSTwoPath(point ParticleVertex input[1], inout PointStream<BillBoardOutput1> outStream1,
inout PointStream<BillBoardOutput2> outStream2, inout PointStream<BillBoardOutput3> outStream3)
{
    switch (input[0].particleType)
    {
        case PARTICLE_EMITTER:{
                BillBoardOutput1 output1;
                BillBoardOutput2 output2;
                BillBoardOutput3 output3;
                output1.position = float3(0.0, 0.0, 0.0);
                output2.uv = float2(0.0, 0.0);
                output3.color = float4(1.0, 1.0, 1.0, 1.0);
                for (int i = 0; i < 6; ++i)
                {
                    outStream1.Append(output1);
                    outStream2.Append(output2);
                    outStream3.Append(output3);
                }
            }
            break;
        case PARTICLE_FLAME:{
                float3 vUp = float3(0.0, 1.0, 0.0);
                float3 vLook = g_camera.cameraEye - input[0].position;
                //vLook.y = 0.0f;
                vLook = normalize(vLook);
                float3 vRight = normalize(cross(vUp, vLook));
                vUp = normalize(cross(vLook, vRight));
                float3 Vertices[4];
                float fHalf = 8.0f - (input[0].lifeTime * 2.0f);
                Vertices[0] = input[0].position + fHalf * vRight - fHalf * vUp;
                Vertices[1] = input[0].position + fHalf * vRight + fHalf * vUp;
                Vertices[2] = input[0].position - fHalf * vRight - fHalf * vUp;
                Vertices[3] = input[0].position - fHalf * vRight + fHalf * vUp;
                float2 UVs[4] = { float2(0.0, 1.0), float2(0.0, 0.0), float2(1.0, 1.0), float2(1.0, 0.0) };
                BillBoardOutput1 output1;
                BillBoardOutput2 output2;
                BillBoardOutput3 output3;
                output3.color = float4(1.0, 1.0, 1.0, 0.5);
                for (int i = 0; i < 6; ++i)
                    outStream3.Append(output3);
            
                output1.position = Vertices[0];
                output2.uv = UVs[0];
                outStream1.Append(output1);
                outStream2.Append(output2);
                output1.position = Vertices[1];
                output2.uv = UVs[1];
                outStream1.Append(output1);
                outStream2.Append(output2);
                output1.position = Vertices[2];
                output2.uv = UVs[2];
                outStream1.Append(output1);
                outStream2.Append(output2);
            
                output1.position = Vertices[1];
                output2.uv = UVs[1];
                outStream1.Append(output1);
                outStream2.Append(output2);
                output1.position = Vertices[3];
                output2.uv = UVs[3];
                outStream1.Append(output1);
                outStream2.Append(output2);
                output1.position = Vertices[2];
                output2.uv = UVs[2];
                outStream1.Append(output1);
                outStream2.Append(output2);
            }
            break;
    }
}

// Magician Laser Particle =======================================================================

[maxvertexcount(42)]
void GS_M_Laser_OnePath(point ParticleVertex input[1], inout PointStream<ParticleVertex> outStream)
{
    ParticleVertex particle = input[0];
    particle.lifeTime += g_camera.fElapsedTime;
    switch (particle.particleType)
    {
        case PARTICLE_EMITTER:{
                particle.direction.x += g_camera.fElapsedTime;
                particle.direction.z += g_camera.fElapsedTime;
                if (particle.lifeTime <= 4.0f)
                {
                    if (particle.direction.x >= 0.1f && particle.direction.z >= 1.0f)
                    {
                        while (particle.direction.x >= 0.1f)
                        {
                            particle.direction.x -= 0.1f;
                        }
                        ParticleVertex output;
                        output.particleType = PARTICLE_FLAME;
                        output.direction = float3(0.0, 0.0, 20.0);
                        output.lifeTime = particle.direction.x;
                        output.position = mul(float4(particle.position + (output.direction * particle.direction.x), 1.0f), g_camera.mtxtarget).xyz;
                        float3 emitP = mul(float4(particle.position, 1.0f), g_camera.mtxtarget).xyz;
                        output.direction = normalize(output.position - emitP) * 40.0f;
                        //output.position = particle.position + (output.direction * particle.direction.x);
                    
                        outStream.Append(particle);
                        outStream.Append(output);
                    }
                    else
                    {
                        outStream.Append(particle);
                    }
                }
            }
            break;
        case PARTICLE_FLAME:{
                if (particle.lifeTime <= 3.0f)
                {
                    particle.position += (particle.direction * g_camera.fElapsedTime);
                    outStream.Append(particle);
                }
            }
            break;
    }
}

[maxvertexcount(252)]
void GS_M_Laser_TwoPath(point ParticleVertex input[1], inout PointStream<BillBoardOutput1> outStream1,
inout PointStream<BillBoardOutput2> outStream2, inout PointStream<BillBoardOutput3> outStream3)
{
    switch (input[0].particleType)
    {
        case PARTICLE_EMITTER:{
                BillBoardOutput1 output1;
                BillBoardOutput2 output2;
                BillBoardOutput3 output3;
                output1.position = float3(0.0, 0.0, 0.0);
                output2.uv = float2(0.0, 0.0);
                output3.color = float4(1.0, 1.0, 1.0, 1.0);
                for (int i = 0; i < 6; ++i)
                {
                    outStream1.Append(output1);
                    outStream2.Append(output2);
                    outStream3.Append(output3);
                }
            }
            break;
        case PARTICLE_FLAME:{
                float3 vUp = float3(0.0, 1.0, 0.0);
                float3 vLook = g_camera.cameraEye - input[0].position;
                //vLook.y = 0.0f;
                vLook = normalize(vLook);
                float3 vRight = normalize(cross(vUp, vLook));
                vUp = normalize(cross(vLook, vRight));
                float3 Vertices[4];
                float fHalf = 1.0f;
                Vertices[0] = input[0].position + fHalf * vRight - fHalf * vUp;
                Vertices[1] = input[0].position + fHalf * vRight + fHalf * vUp;
                Vertices[2] = input[0].position - fHalf * vRight - fHalf * vUp;
                Vertices[3] = input[0].position - fHalf * vRight + fHalf * vUp;
                float2 UVs[4] = { float2(0.0, 1.0), float2(0.0, 0.0), float2(1.0, 1.0), float2(1.0, 0.0) };
                BillBoardOutput1 output1;
                BillBoardOutput2 output2;
                BillBoardOutput3 output3;
                output3.color = float4(1.0, 1.0, 1.0, 0.5);
                for (int i = 0; i < 6; ++i)
                    outStream3.Append(output3);
            
                output1.position = Vertices[0];
                output2.uv = UVs[0];
                outStream1.Append(output1);
                outStream2.Append(output2);
                output1.position = Vertices[1];
                output2.uv = UVs[1];
                outStream1.Append(output1);
                outStream2.Append(output2);
                output1.position = Vertices[2];
                output2.uv = UVs[2];
                outStream1.Append(output1);
                outStream2.Append(output2);
            
                output1.position = Vertices[1];
                output2.uv = UVs[1];
                outStream1.Append(output1);
                outStream2.Append(output2);
                output1.position = Vertices[3];
                output2.uv = UVs[3];
                outStream1.Append(output1);
                outStream2.Append(output2);
                output1.position = Vertices[2];
                output2.uv = UVs[2];
                outStream1.Append(output1);
                outStream2.Append(output2);
            }
            break;
    }
}

// ===============================================================================================

// Exeplosive Particle =======================================================================

[maxvertexcount(42)]
void GS_Boom_OnePath(point ParticleVertex input[1], inout PointStream<ParticleVertex> outStream)
{
    ParticleVertex particle = input[0];
    particle.lifeTime += g_camera.fElapsedTime;
    switch (particle.particleType)
    {
        case PARTICLE_EMITTER:{
                for (uint i = 0; i < 6; ++i)
                {
                    for (uint j = 0; j < 6; ++j)
                    {
                        ParticleVertex output;
                        output.position = mul(float4(particle.position, 1.0f), g_camera.mtxtarget).xyz;
                        output.lifeTime = 0.0f;
                        output.particleType = PARTICLE_FLAME;
                        output.direction = normalize(float3(cos(radians(60.0f * j)), sin(radians((30.0f * i) - 75.0f)), sin(radians(60.0f * j)))) * 3.0f;

                        outStream.Append(output);
                    }
                }
            }
            break;
        case PARTICLE_FLAME:{
                if (particle.lifeTime <= 3.0f)
                {
                    particle.position += (5 * particle.direction * g_camera.fElapsedTime);
                    outStream.Append(particle);
                }
            }
            break;
    }
}

[maxvertexcount(252)]
void GS_Boom_TwoPath(point ParticleVertex input[1], inout PointStream<BillBoardOutput1> outStream1,
inout PointStream<BillBoardOutput2> outStream2, inout PointStream<BillBoardOutput3> outStream3)
{
    switch (input[0].particleType)
    {
        case PARTICLE_EMITTER:{
                BillBoardOutput1 output1;
                BillBoardOutput2 output2;
                BillBoardOutput3 output3;
                output1.position = float3(0.0, 0.0, 0.0);
                output2.uv = float2(0.0, 0.0);
                output3.color = float4(1.0, 1.0, 1.0, 1.0);
                for (int i = 0; i < 6; ++i)
                {
                    outStream1.Append(output1);
                    outStream2.Append(output2);
                    outStream3.Append(output3);
                }
            }
            break;
        case PARTICLE_FLAME:{
                float3 vUp = float3(0.0, 1.0, 0.0);
                float3 vLook = g_camera.cameraEye - input[0].position;
                //vLook.y = 0.0f;
                vLook = normalize(vLook);
                float3 vRight = normalize(cross(vUp, vLook));
                vUp = normalize(cross(vLook, vRight));
                float3 Vertices[4];
                float fHalf = 0.5f;
                Vertices[0] = input[0].position + fHalf * vRight - fHalf * vUp;
                Vertices[1] = input[0].position + fHalf * vRight + fHalf * vUp;
                Vertices[2] = input[0].position - fHalf * vRight - fHalf * vUp;
                Vertices[3] = input[0].position - fHalf * vRight + fHalf * vUp;
                float2 UVs[4] = { float2(0.0, 1.0), float2(0.0, 0.0), float2(1.0, 1.0), float2(1.0, 0.0) };
                BillBoardOutput1 output1;
                BillBoardOutput2 output2;
                BillBoardOutput3 output3;
                output3.color = float4(1.0, 1.0, 1.0, 1.0);
                for (int i = 0; i < 6; ++i)
                    outStream3.Append(output3);
            
                output1.position = Vertices[0];
                output2.uv = UVs[0];
                outStream1.Append(output1);
                outStream2.Append(output2);
                output1.position = Vertices[1];
                output2.uv = UVs[1];
                outStream1.Append(output1);
                outStream2.Append(output2);
                output1.position = Vertices[2];
                output2.uv = UVs[2];
                outStream1.Append(output1);
                outStream2.Append(output2);
            
                output1.position = Vertices[1];
                output2.uv = UVs[1];
                outStream1.Append(output1);
                outStream2.Append(output2);
                output1.position = Vertices[3];
                output2.uv = UVs[3];
                outStream1.Append(output1);
                outStream2.Append(output2);
                output1.position = Vertices[2];
                output2.uv = UVs[2];
                outStream1.Append(output1);
                outStream2.Append(output2);
            }
            break;
    }
}

// ===============================================================================================

// Buff Effect Particle =======================================================================

[maxvertexcount(42)]
void GS_Buff_OnePath(point ParticleVertex input[1], inout PointStream<ParticleVertex> outStream)
{
    ParticleVertex particle = input[0];
    particle.lifeTime += g_camera.fElapsedTime;
    switch (particle.particleType)
    {
        case PARTICLE_EMITTER:{
                particle.direction.y += 360.0f * g_camera.fElapsedTime;
                if (particle.lifeTime <= 2.0f)
                {
                    if (particle.direction.y >= 36.0f)
                    {
                        while (particle.direction.y >= 36.0f)
                        {
                            particle.direction.y -= 36.0f;
                        }
                        ParticleVertex output;
                        output.particleType = PARTICLE_FLAME;
                        output.direction = float3(0.0, 8.0, 0.0);
                        output.lifeTime = particle.direction.y / 360.0f;
                        float ttime = particle.lifeTime;
                        uint cnt = 0;
                        while (ttime >= 0.1f)
                        {
                            ttime -= 0.1f;
                            ++cnt;
                        }
                        output.position = mul(float4(particle.position + (float3(cos(radians(36.0f * cnt)), 0.0, sin(radians(36.0f * cnt))) * 1.0f) + (output.direction * output.lifeTime), 1.0f), g_camera.mtxtarget).xyz;
                    
                        outStream.Append(particle);
                        outStream.Append(output);
                    }
                    else
                    {
                        outStream.Append(particle);
                    }
                }
            }
            break;
        case PARTICLE_FLAME:{
                if (particle.lifeTime <= 1.0f)
                {
                    particle.position += (particle.direction * g_camera.fElapsedTime);
                    outStream.Append(particle);
                }
            }
            break;
    }
}

[maxvertexcount(252)]
void GS_Buff_TwoPath(point ParticleVertex input[1], inout PointStream<BillBoardOutput1> outStream1,
inout PointStream<BillBoardOutput2> outStream2, inout PointStream<BillBoardOutput3> outStream3)
{
    switch (input[0].particleType)
    {
        case PARTICLE_EMITTER:{
                BillBoardOutput1 output1;
                BillBoardOutput2 output2;
                BillBoardOutput3 output3;
                output1.position = float3(0.0, 0.0, 0.0);
                output2.uv = float2(0.0, 0.0);
                output3.color = float4(1.0, 1.0, 1.0, 1.0);
                for (int i = 0; i < 6; ++i)
                {
                    outStream1.Append(output1);
                    outStream2.Append(output2);
                    outStream3.Append(output3);
                }
            }
            break;
        case PARTICLE_FLAME:{
                float3 vUp = float3(0.0, 1.0, 0.0);
                float3 vLook = g_camera.cameraEye - input[0].position;
                //vLook.y = 0.0f;
                vLook = normalize(vLook);
                float3 vRight = normalize(cross(vUp, vLook));
                vUp = normalize(cross(vLook, vRight));
                float3 Vertices[4];
                float fHalf = 0.5f;
                Vertices[0] = input[0].position + fHalf * vRight - fHalf * vUp;
                Vertices[1] = input[0].position + fHalf * vRight + fHalf * vUp;
                Vertices[2] = input[0].position - fHalf * vRight - fHalf * vUp;
                Vertices[3] = input[0].position - fHalf * vRight + fHalf * vUp;
                float2 UVs[4] = { float2(0.0, 1.0), float2(0.0, 0.0), float2(1.0, 1.0), float2(1.0, 0.0) };
                BillBoardOutput1 output1;
                BillBoardOutput2 output2;
                BillBoardOutput3 output3;
                output3.color = float4(1.0, 1.0, 1.0, 0.5);
                for (int i = 0; i < 6; ++i)
                    outStream3.Append(output3);
            
                output1.position = Vertices[0];
                output2.uv = UVs[0];
                outStream1.Append(output1);
                outStream2.Append(output2);
                output1.position = Vertices[1];
                output2.uv = UVs[1];
                outStream1.Append(output1);
                outStream2.Append(output2);
                output1.position = Vertices[2];
                output2.uv = UVs[2];
                outStream1.Append(output1);
                outStream2.Append(output2);
            
                output1.position = Vertices[1];
                output2.uv = UVs[1];
                outStream1.Append(output1);
                outStream2.Append(output2);
                output1.position = Vertices[3];
                output2.uv = UVs[3];
                outStream1.Append(output1);
                outStream2.Append(output2);
                output1.position = Vertices[2];
                output2.uv = UVs[2];
                outStream1.Append(output1);
                outStream2.Append(output2);
            }
            break;
    }
}

// ===============================================================================================