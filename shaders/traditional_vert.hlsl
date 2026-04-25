// The output structure that will be passed to the Fragment Shader
struct VSOutput {
    float4 Pos : SV_POSITION;
    float3 Color : COLOR;
};

// =========================================================================
// --- THE FIX: STRICT DXC PUSH CONSTANT SYNTAX ---
// =========================================================================
// We must define a struct, and then declare a single global variable (pc) 
// with the [[vk::push_constant]] attribute attached directly to it.
struct PushConstants {
    float4x4 viewProj;
};
[[vk::push_constant]] PushConstants pc;

// SV_VertexID is automatically provided by the GPU, counting upwards
VSOutput main(uint VertexIndex : SV_VertexID) {
    VSOutput output;
    
    // Positioned at Z = -1.5, right in front of the camera lens!
    float3 localPos = float3(0.0, 0.0, 0.0);
    if (VertexIndex == 0) { 
        localPos = float3(0.0, -1.0, -1.5);  // Top
        output.Color = float3(1.0, 0.0, 0.0);      
    }
    else if (VertexIndex == 1) { 
        localPos = float3(1.0, 1.0, -1.5);   // Bottom Right
        output.Color = float3(0.0, 1.0, 0.0);      
    }
    else { 
        localPos = float3(-1.0, 1.0, -1.5);  // Bottom Left
        output.Color = float3(0.0, 0.0, 1.0);      
    }
    
    // Multiply the matrix by the vector. Because both C++ GLM and Vulkan DXC 
    // natively use Column-Major packing, this standard multiplication works perfectly.
    output.Pos = mul(pc.viewProj, float4(localPos, 1.0));
    
    return output;
}