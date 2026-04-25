RWStructuredBuffer<float> ssbo_data : register(u0); [numthreads(1, 1, 1)] void main(uint3 id : SV_DispatchThreadID) { ssbo_data[id.x] = 1.0; }
