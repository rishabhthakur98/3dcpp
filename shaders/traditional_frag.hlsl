struct VSOutput {
    float4 Pos : SV_POSITION;
    float3 WorldPos : POSITION0;
    float2 UV : TEXCOORD;
    float4 Color : COLOR0;
    float3 TangentViewPos  : POSITION1;
    float3 TangentFragPos  : POSITION2;
    float3x3 TBN           : MATRIX;
};

// Must perfectly match the 16-byte alignment from the C++ SSBO struct!
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

[[vk::binding(0, 1)]] Texture2D albedoTex;
[[vk::binding(0, 1)]] SamplerState samp;
[[vk::binding(1, 1)]] Texture2D normalTex;
[[vk::binding(2, 1)]] Texture2D metRoughTex;
[[vk::binding(3, 1)]] Texture2D heightTex;

static const float PI = 3.14159265359;

float2 ParallaxMapping(float2 uv, float3 viewDir) {
    if (globalBuffer[0].usePOM == 0) return uv;
    
    float numLayers = lerp(32.0, 8.0, abs(dot(float3(0, 0, 1), viewDir)));
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    
    float2 P = (viewDir.xy / max(viewDir.z, 0.001)) * globalBuffer[0].pomScale; 
    float2 deltaTexCoords = P / numLayers;
    
    float2 currentTexCoords = uv;
    float currentDepthMapValue = 1.0 - heightTex.Sample(samp, currentTexCoords).r;
    
    while(currentLayerDepth < currentDepthMapValue) {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = 1.0 - heightTex.Sample(samp, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }
    
    float2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = (1.0 - heightTex.Sample(samp, prevTexCoords).r) - currentLayerDepth + layerDepth;
    
    float weight = afterDepth / (afterDepth - beforeDepth);
    return prevTexCoords * weight + currentTexCoords * (1.0 - weight);
}

float DistributionGGX(float3 N, float3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float4 main(VSOutput input) : SV_TARGET {
    float3 viewDirTangent = normalize(input.TangentViewPos - input.TangentFragPos);
    
    float2 texCoords = ParallaxMapping(input.UV, viewDirTangent);
    
    float4 albedoData = albedoTex.Sample(samp, texCoords) * input.Color;
    float3 albedo = pow(albedoData.rgb, 2.2); 
    
    if (globalBuffer[0].usePBR == 0) return float4(albedo, albedoData.a);

    float3 tangentNormal = normalTex.Sample(samp, texCoords).rgb * 2.0 - 1.0;
    float3 N = normalize(mul(tangentNormal, input.TBN));
    
    float4 metRough = metRoughTex.Sample(samp, texCoords);
    float roughness = metRough.g;
    float metallic = metRough.b;

    float3 V = normalize(globalBuffer[0].camPos.xyz - input.WorldPos);
    
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);
    
    float3 lightPos = float3(50.0, 100.0, 50.0);
    float3 lightColor = float3(30000.0, 30000.0, 30000.0);
    
    float3 L = normalize(lightPos - input.WorldPos);
    float3 H = normalize(V + L);
    
    float distance = length(lightPos - input.WorldPos);
    float attenuation = 1.0 / (distance * distance);
    float3 radiance = lightColor * attenuation;

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);      
    float3 F  = FresnelSchlick(max(dot(H, V), 0.0), F0);
    
    float3 numerator    = NDF * G * F;
    float denominator   = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; 
    float3 specular     = numerator / denominator;
    
    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallic;	  

    float NdotL = max(dot(N, L), 0.0);
    float3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    float3 ambient = float3(0.03, 0.03, 0.03) * albedo * metRough.r; 
    float3 color = ambient + Lo;

    color = color / (color + float3(1.0, 1.0, 1.0));
    color = pow(color, float3(1.0/2.2, 1.0/2.2, 1.0/2.2)); 

    return float4(color, albedoData.a);
}