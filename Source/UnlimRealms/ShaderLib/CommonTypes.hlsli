///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef COMMON_TYPES_HLSLI
#define COMMON_TYPES_HLSLI

// cpu/gpu compatible declarations

// base types
#if defined(CPP_CODE)

#define CFLOAT(v) ur_float v
#define CFLOAT2(v) ur_float2 v
#define CFLOAT3(v) ur_float3 v
#define CFLOAT4(v) ur_float4 v
#define CFLOAT4x4(v) ur_float4x4 v
#define CFLOAT3x4(v) ur_float v[12]
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
#define CFLOAT4x4(v) float4x4 v
#define CFLOAT3x4(v) float3x4 v
#define CINT(v) int v
#define CINT2(v) int2 v
#define CINT3(v) int3 v
#define CINT4(v) int4 v
#define CUINT(v) uint v
#define CUINT2(v) uint2 v
#define CUINT3(v) uint3 v
#define CUINT4(v) uint4 v

#endif

// shader binding descriptors
#if defined(CPP_CODE)

#define DECRIPTOR_NAME(name) name##Descriptor
#define DECLARE_DESCRIPTOR_CONSTANT(name, regIdx) const ur_uint32 DECRIPTOR_NAME(name) = regIdx
#define DECLARE_GRAF_DESCRIPTOR_RANGE_DESC(descriptorType, name) const GrafDescriptorRangeDesc name##DescriptorRange = { GrafDescriptorType::##descriptorType, DECRIPTOR_NAME(name), 1 }

#define DESCRIPTOR_ConstantBuffer(dataType, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_RANGE_DESC(ConstantBuffer, name);
#define DESCRIPTOR_Sampler(name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_RANGE_DESC(Sampler, name);
#define DESCRIPTOR_AccelerationStructure(name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_RANGE_DESC(AccelerationStructure, name);
#define DESCRIPTOR_Texture1D(dataType, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_RANGE_DESC(Texture, name);
#define DESCRIPTOR_Texture2D(dataType, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_RANGE_DESC(Texture, name);
#define DESCRIPTOR_Texture3D(dataType, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_RANGE_DESC(Texture, name);
#define DESCRIPTOR_RWTexture1D(dataType, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_RANGE_DESC(RWTexture, name);
#define DESCRIPTOR_RWTexture2D(dataType, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_RANGE_DESC(RWTexture, name);
#define DESCRIPTOR_RWTexture3D(dataType, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_RANGE_DESC(RWTexture, name);
#define DESCRIPTOR_ByteAddressBuffer(name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_RANGE_DESC(Buffer, name);
#define DESCRIPTOR_RWByteAddressBuffer(name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_RANGE_DESC(RWBuffer, name);
#define DESCRIPTOR_StructuredBuffer(dataType, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_RANGE_DESC(Buffer, name);
#define DESCRIPTOR_RWStructuredBuffer(dataType, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_RANGE_DESC(RWBuffer, name);

#else

#define DESCRIPTOR_ConstantBuffer(dataType, name, regIdx) ConstantBuffer<dataType> name : register(b##regIdx)
#define DESCRIPTOR_Sampler(name, regIdx) sampler name : register(s##regIdx)
#define DESCRIPTOR_AccelerationStructure(name, regIdx) RaytracingAccelerationStructure name : register(t##regIdx)
#define DESCRIPTOR_Texture1D(dataType, name, regIdx) Texture1D<dataType> name : register(t##regIdx)
#define DESCRIPTOR_Texture2D(dataType, name, regIdx) Texture2D<dataType> name : register(t##regIdx)
#define DESCRIPTOR_Texture3D(dataType, name, regIdx) Texture3D<dataType> name : register(t##regIdx)
#define DESCRIPTOR_RWTexture1D(dataType, name, regIdx) RWTexture1D<dataType> name : register(u##regIdx)
#define DESCRIPTOR_RWTexture2D(dataType, name, regIdx) RWTexture2D<dataType> name : register(u##regIdx)
#define DESCRIPTOR_RWTexture3D(dataType, name, regIdx) RWTexture3D<dataType> name : register(u##regIdx)
#define DESCRIPTOR_ByteAddressBuffer(name, regIdx) ByteAddressBuffer name : register(t##regIdx)
#define DESCRIPTOR_RWByteAddressBuffer(name, regIdx) RWByteAddressBuffer name : register(u##regIdx)
#define DESCRIPTOR_StructuredBuffer(dataType, name, regIdx) StructuredBuffer<dataType> name : register(t##regIdx)
#define DESCRIPTOR_RWStructuredBuffer(dataType, name, regIdx) RWStructuredBuffer<dataType> name : register(u##regIdx)

#endif

#endif