struct VSOutput {
    float4 Pos : SV_POSITION;
    float3 Color : COLOR;
};

[[vk::push_constant]]
struct PushConstants {
    float4x4 viewProj;
} pc;

VSOutput main(uint VertexIndex : SV_VertexID) {
    VSOutput output;
    
    float3 localPos = float3(0.0, 0.0, 0.0);
    
    // Ordered CCW to match glTF and AAA engine standards
    if (VertexIndex == 0) { 
        localPos = float3(0.0, -1.0, -1.5);  // Top
        output.Color = float3(1.0, 0.0, 0.0);      
    }
    else if (VertexIndex == 1) { 
        localPos = float3(-1.0, 1.0, -1.5);  // Bottom Left
        output.Color = float3(0.0, 0.0, 1.0);      
    }
    else { 
        localPos = float3(1.0, 1.0, -1.5);   // Bottom Right
        output.Color = float3(0.0, 1.0, 0.0);      
    }
    
    output.Pos = mul(pc.viewProj, float4(localPos, 1.0));
    return output;
}