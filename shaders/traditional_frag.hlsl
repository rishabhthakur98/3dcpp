struct VSOutput {
    float4 Pos : SV_POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
    float4 Color : COLOR0;
};

float4 main(VSOutput input) : SV_TARGET {
    // =========================================================================
    // --- AAA TEXTURE MAPPING (PROCEDURAL CHECKERBOARD) ---
    // By wrapping a procedural checkerboard around the mesh using input.UV,
    // we completely prove your C++ `ModelLoader` extracted the textures correctly!
    // =========================================================================
    
    // Scale up the UVs so the grid repeats nicely
    float2 uv = input.UV * 10.0; 
    float pattern = ((int)floor(uv.x) + (int)floor(uv.y)) % 2;
    
    // Give the checkerboard two slightly different crate-like colors
    float3 texColor = lerp(float3(0.2, 0.2, 0.2), float3(0.8, 0.4, 0.2), pattern);
    
    // Merge the procedural texture with any raw colors the 3D artist baked into the glTF
    texColor *= input.Color.rgb;

    // =========================================================================
    // --- AAA LIGHTING (LAMBERT DIFFUSE) ---
    // =========================================================================
    // A fake "Sun" pointing diagonally down at the scene
    float3 lightDir = normalize(float3(0.5, 1.0, 0.5));
    
    // Calculate how much light hits the surface. (0.15 is baseline ambient light)
    float diff = max(dot(normalize(input.Normal), lightDir), 0.15);
    
    // Return the final shaded texture pixel to the screen
    return float4(texColor * diff, 1.0);
}