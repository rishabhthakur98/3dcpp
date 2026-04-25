// The output structure that will be passed to the Fragment Shader
struct VSOutput {
    float4 Pos : SV_POSITION;
    float3 Color : COLOR;
};

// SV_VertexID is automatically provided by the GPU, counting from 0 upwards as it draws
VSOutput main(uint VertexIndex : SV_VertexID) {
    VSOutput output;
    
    // Hardcode the coordinates and colors for 3 points of a triangle
    if (VertexIndex == 0) { 
        output.Pos = float4(0.0, -0.5, 0.0, 1.0);  // Top Center
        output.Color = float3(1.0, 0.0, 0.0);      // Red
    }
    else if (VertexIndex == 1) { 
        output.Pos = float4(0.5, 0.5, 0.0, 1.0);   // Bottom Right
        output.Color = float3(0.0, 1.0, 0.0);      // Green
    }
    else { 
        output.Pos = float4(-0.5, 0.5, 0.0, 1.0);  // Bottom Left
        output.Color = float3(0.0, 0.0, 1.0);      // Blue
    }
    
    return output;
}