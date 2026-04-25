struct VSOutput {
    float4 Pos : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
    float4 Color : COLOR0;
};

// Texture Sampler binding exactly matches our Descriptor Set configuration
[[vk::binding(0, 0)]] Texture2D tex;
[[vk::binding(0, 0)]] SamplerState samp;

float4 main(VSOutput input) : SV_TARGET {
    // Read the exact pixel from the VRAM PNG texture using UVs
    float4 texColor = tex.Sample(samp, input.UV) * input.Color;
    
    // AAA Diffuse Lighting System
    float3 lightDir = normalize(float3(0.5, 1.0, 0.5));
    float diff = max(dot(normalize(input.Normal), lightDir), 0.15);
    
    // Apply shading and output the pixel to the swapchain
    return float4(texColor.rgb * diff, texColor.a);
}