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

// Global SSBO Configuration
struct GlobalData {
    float4x4 viewProj;
    float4 camPos;
    float pomScale;
    int usePOM;
    int usePBR;
    float padding;
};
[[vk::binding(0, 0)]] StructuredBuffer<GlobalData> globalBuffer;

// Dynamic Push Constants for per-model translation
struct PushConstants {
    float4x4 model;
};
[[vk::push_constant]] PushConstants pc;

VSOutput main(VSInput input) {
    VSOutput output;
    
    // Transform vertex position to world space
    output.WorldPos = mul(pc.model, float4(input.Position, 1.0)).xyz;
    
    // Transform from world space to projection space using the camera
    output.Pos = mul(globalBuffer[0].viewProj, float4(output.WorldPos, 1.0));
    
    output.UV = input.UV;
    output.Color = input.Color;
    
    // Calculate Tangent Space (TBN) Matrix
    float3x3 normalMatrix = (float3x3)pc.model; 
    float3 T = normalize(mul(normalMatrix, input.Tangent.xyz));
    float3 N = normalize(mul(normalMatrix, input.Normal));
    
    // Re-orthogonalize T with respect to N using Gram-Schmidt process
    T = normalize(T - dot(T, N) * N);
    
    // Calculate Bitangent
    float3 B = cross(N, T) * input.Tangent.w; 
    
    // Construct the TBN Matrix (World Space to Tangent Space)
    output.TBN = float3x3(T, B, N);
    
    // Transform the Camera and Fragment positions into Tangent Space
    // We multiply using output.TBN directly, NOT the inverse, to enter Tangent Space!
    output.TangentViewPos = mul(output.TBN, globalBuffer[0].camPos.xyz);
    output.TangentFragPos = mul(output.TBN, output.WorldPos);
    
    return output;
}