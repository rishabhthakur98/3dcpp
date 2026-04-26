struct VSInput {
    [[vk::location(0)]] float3 Position : POSITION;
    [[vk::location(1)]] float3 Normal   : NORMAL;
    [[vk::location(2)]] float2 UV       : TEXCOORD;
    [[vk::location(3)]] float4 Color    : COLOR0;
    [[vk::location(4)]] float4 Tangent  : TANGENT;
};

struct VSOutput {
    float4 Pos : SV_POSITION;
    float3 WorldPos : POSITION0;
    float2 UV : TEXCOORD;
    float4 Color : COLOR0;
    float3 TangentViewPos  : POSITION1;
    float3 TangentFragPos  : POSITION2;
    float3x3 TBN           : MATRIX;
};

struct GlobalData {
    float4x4 viewProj;
    float4 camPos;
    float pomScale;
    int usePOM;
    int usePBR;
    int useDisplacement;
    float displacementScale;
    float3 padding;
};

[[vk::binding(0, 0)]] StructuredBuffer<GlobalData> globalBuffer;

// --- VERTEX SHADER TEXTURE ACCESS ---
// This allows the vertex shader to read the height map directly to warp the 3D geometry
[[vk::binding(3, 1)]] Texture2D heightTex;
[[vk::binding(3, 1)]] SamplerState samp;

struct PushConstants {
    float4x4 model;
};
[[vk::push_constant]] PushConstants pc;

VSOutput main(VSInput input) {
    VSOutput output;
    
    // We start with the original vertex position
    float3 localPos = input.Position;

    // --- HARDWARE VERTEX DISPLACEMENT ---
    // If the user checked "Enable Hardware Vertex Displacement" in the UI, physically push
    // the geometry outward along its normals!
    if (globalBuffer[0].useDisplacement == 1) {
        // Vertex shaders must use SampleLevel because they have no screen-space derivatives for automatic mipmaps
        float height = heightTex.SampleLevel(samp, input.UV, 0).r;
        localPos += input.Normal * (height * globalBuffer[0].displacementScale);
    }
    
    // Transform displaced vertex position to world space
    output.WorldPos = mul(pc.model, float4(localPos, 1.0)).xyz;
    
    // Transform from world space to projection space using the camera
    output.Pos = mul(globalBuffer[0].viewProj, float4(output.WorldPos, 1.0));
    
    output.UV = input.UV;
    output.Color = input.Color;
    
    float3x3 normalMatrix = (float3x3)pc.model; 
    float3 T = normalize(mul(normalMatrix, input.Tangent.xyz));
    float3 N = normalize(mul(normalMatrix, input.Normal));
    
    T = normalize(T - dot(T, N) * N);
    float3 B = cross(N, T) * input.Tangent.w; 
    
    output.TBN = float3x3(T, B, N);
    
    output.TangentViewPos = mul(output.TBN, globalBuffer[0].camPos.xyz);
    output.TangentFragPos = mul(output.TBN, output.WorldPos);
    
    return output;
}