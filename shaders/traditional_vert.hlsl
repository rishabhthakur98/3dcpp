struct VSInput {
    [[vk::location(0)]] float3 Position : POSITION;
    [[vk::location(1)]] float3 Normal   : NORMAL;
    [[vk::location(2)]] float2 UV       : TEXCOORD;
    [[vk::location(3)]] float4 Color    : COLOR0;
    [[vk::location(4)]] float4 Tangent  : TANGENT;
};

// --- PIPELINE DATA BUS ---
struct VSOutput {
    float4 Pos : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
    float4 Color : COLOR0;
};

struct PushConstants {
    float4x4 viewProj;
    float4x4 model;
};
[[vk::push_constant]] PushConstants pc;

VSOutput main(VSInput input) {
    VSOutput output;
    
    float4 worldPos = mul(pc.model, float4(input.Position, 1.0));
    output.Pos = mul(pc.viewProj, worldPos);
    
    // Rotate the model's normal vectors by the matrix so lighting reacts 
    // correctly if the crate is rotated upside down!
    output.Normal = normalize(mul((float3x3)pc.model, input.Normal));
    
    // Pass the extracted UV coordinates straight to the fragment shader
    output.UV = input.UV;
    output.Color = input.Color;
    
    return output;
}