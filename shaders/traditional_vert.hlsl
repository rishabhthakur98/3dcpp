// --- DYNAMIC VERTEX INJECTION ---
struct VSInput {
    [[vk::location(0)]] float3 Position : POSITION;
    [[vk::location(1)]] float3 Normal   : NORMAL;
    [[vk::location(2)]] float2 UV       : TEXCOORD;
    [[vk::location(3)]] float4 Color    : COLOR0;
    [[vk::location(4)]] float4 Tangent  : TANGENT;
};

struct VSOutput {
    float4 Pos : SV_POSITION;
    float3 Color : COLOR;
};

// We expanded the fast block to hold BOTH the Camera and the Object's location!
struct PushConstants {
    float4x4 viewProj;
    float4x4 model;
};
[[vk::push_constant]] PushConstants pc;

VSOutput main(VSInput input) {
    VSOutput output;
    
    // 1. Move the raw vertex to its designated location in the world
    float4 worldPos = mul(pc.model, float4(input.Position, 1.0));
    
    // 2. Project it onto the camera lens
    output.Pos = mul(pc.viewProj, worldPos);
    
    // 3. For now, we will draw the Normal vectors as colors so you can see beautiful 3D shading!
    output.Color = input.Normal * 0.5 + 0.5;
    
    return output;
}