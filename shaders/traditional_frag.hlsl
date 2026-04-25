// The input structure must perfectly match the output of the Vertex Shader
struct PSInput {
    float4 Pos : SV_POSITION;
    float3 Color : COLOR;
};

// SV_TARGET tells the GPU to output this color to our Swapchain Framebuffer
float4 main(PSInput input) : SV_TARGET {
    // We append 1.0f for the Alpha (opacity) channel
    return float4(input.Color, 1.0f);
}