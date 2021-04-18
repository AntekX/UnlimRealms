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

struct UR_DECL GrafDescriptorWrapper
{
	// simple wrapper allowing to use declared constant range both as a layout initializer and a binding slot index
	GrafDescriptorRangeDesc Range;
	operator const GrafDescriptorRangeDesc& () const { return Range; }
	operator ur_uint32 () const { return Range.BindingOffset; }
};

#define DECRIPTOR_INDEX_NAME(name) name##DescriptorIdx
#define DECRIPTOR_WRAPPER_NAME(name) name##Descriptor
#define DECLARE_DESCRIPTOR_CONSTANT(name, regIdx) const ur_uint32 DECRIPTOR_INDEX_NAME(name) = regIdx
#define DECLARE_GRAF_DESCRIPTOR_RANGE_WRAPPER(descriptorType, name, count) const GrafDescriptorWrapper DECRIPTOR_WRAPPER_NAME(name) = { GrafDescriptorType::##descriptorType, DECRIPTOR_INDEX_NAME(name), count }
#define DECLARE_GRAF_DESCRIPTOR_WRAPPER(descriptorType, name) DECLARE_GRAF_DESCRIPTOR_RANGE_WRAPPER(descriptorType, name, 1)

#define DESCRIPTOR_ConstantBuffer(dataType, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_WRAPPER(ConstantBuffer, name);
#define DESCRIPTOR_Sampler(name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_WRAPPER(Sampler, name);
#define DESCRIPTOR_AccelerationStructure(name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_WRAPPER(AccelerationStructure, name);
#define DESCRIPTOR_Texture1D(dataType, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_WRAPPER(Texture, name);
#define DESCRIPTOR_Texture2D(dataType, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_WRAPPER(Texture, name);
#define DESCRIPTOR_Texture3D(dataType, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_WRAPPER(Texture, name);
#define DESCRIPTOR_RWTexture1D(dataType, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_WRAPPER(RWTexture, name);
#define DESCRIPTOR_RWTexture2D(dataType, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_WRAPPER(RWTexture, name);
#define DESCRIPTOR_RWTexture3D(dataType, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_WRAPPER(RWTexture, name);
#define DESCRIPTOR_ByteAddressBuffer(name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_WRAPPER(Buffer, name);
#define DESCRIPTOR_RWByteAddressBuffer(name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_WRAPPER(RWBuffer, name);
#define DESCRIPTOR_StructuredBuffer(dataType, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_WRAPPER(Buffer, name);
#define DESCRIPTOR_RWStructuredBuffer(dataType, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_WRAPPER(RWBuffer, name);
#define DESCRIPTOR_ARRAY_Texture2D(maxSize, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_RANGE_WRAPPER(TextureDynamicArray, name, maxSize);
#define DESCRIPTOR_ARRAY_ByteAddressBuffer(maxSize, name, regIdx) DECLARE_DESCRIPTOR_CONSTANT(name, regIdx); DECLARE_GRAF_DESCRIPTOR_RANGE_WRAPPER(BufferDynamicArray, name, maxSize);

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
#define DESCRIPTOR_ARRAY_Texture2D(maxSize, name, regIdx) Texture2D name[maxSize] : register(t##regIdx)
#define DESCRIPTOR_ARRAY_ByteAddressBuffer(maxSize, name, regIdx) ByteAddressBuffer name[maxSize] : register(t##regIdx)

#endif

#endif