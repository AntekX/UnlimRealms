#ifndef COMMON_TYPES_HLSLI
#define COMMON_TYPES_HLSLI

// cpu/gpu compatible declarations
#if defined(CPP_CODE)

#define CFLOAT(v) ur_float v
#define CFLOAT2(v) ur_float v[2]
#define CFLOAT3(v) ur_float v[3]
#define CFLOAT4(v) ur_float v[4]
#define CFLOAT2x2(v) ur_float v[2][2]
#define CFLOAT3x3(v) ur_float v[3][3]
#define CFLOAT4x4(v) ur_float v[4][4]
#define CINT(v) ur_int32 v
#define CINT2(v) ur_int32 v[2]
#define CINT3(v) ur_int32 v[3]
#define CINT4(v) ur_int32 v[4]
#define CUINT(v) ur_uint32 v
#define CUINT2(v) ur_uint32 v[2]
#define CUINT3(v) ur_uint32 v[3]
#define CUINT4(v) ur_uint32 v[4]

#else

#define CFLOAT(v) float v
#define CFLOAT2(v) float2 v
#define CFLOAT3(v) float3 v
#define CFLOAT4(v) float4 v
#define CFLOAT2x2(v) float2x2 v
#define CFLOAT3x3(v) float3x3 v
#define CFLOAT4x4(v) float4x4 v
#define CINT(v) int v
#define CINT2(v) int2 v
#define CINT3(v) int3 v
#define CINT4(v) int4 v
#define CUINT(v) uint v
#define CUINT2(v) uint2 v
#define CUINT3(v) uint3 v
#define CUINT4(v) uint4 v

#endif

#endif