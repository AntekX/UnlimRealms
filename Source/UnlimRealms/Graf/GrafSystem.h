///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GRAF: GRaphics Abstraction Front-end
// Low level graphics APIs abstraction layer
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "UnlimRealms.h"

#if defined(_DEBUG)
#define UR_GRAF_LOG_LEVEL_DEBUG
#endif

#if defined(UR_GRAF_LOG_LEVEL_DEBUG)
#define LogNoteGrafDbg(text) GetRealm().GetLog().WriteLine(text, Log::Note)
#else
#define LogNoteGrafDbg(text)
#endif

namespace UnlimRealms
{

	// forward declarations
	class GrafSystem;
	class GrafDevice;
	class GrafCommandList;
	class GrafFence;
	class GrafCanvas;
	class GrafImage;
	class GrafImageSubresource;
	class GrafBuffer;
	class GrafSampler;
	class GrafShader;
	class GrafShaderLib;
	class GrafRenderPass;
	class GrafRenderTarget;
	class GrafDescriptorTableLayout;
	class GrafDescriptorTable;
	class GrafPipeline;
	class GrafComputePipeline;
	class GrafRayTracingPipeline;
	class GrafAccelerationStructure;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafRayTracingProperties
	{
		ur_bool RayTraceSupported;
		ur_bool RayQuerySupported;
		ur_uint32 ShaderGroupHandleSize;
		ur_uint32 ShaderGroupBaseAlignment;
		ur_uint32 RecursionDepthMax;
		ur_uint64 InstanceCountMax;
		ur_uint64 PrimitiveCountMax;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafPhysicalDeviceDesc
	{
		std::string Description;
		ur_uint VendorId;
		ur_uint DeviceId;
		ur_size LocalMemory; // device memory total ecxlusive and shared
		ur_size LocalMemoryExclusive; // device memory not visible to host
		ur_size LocalMemoryHostVisible; // device memory visible to host
		ur_size SystemMemory; // part of system memory dedicated to or shared with device
		ur_size ConstantBufferOffsetAlignment;
		ur_size ImageDataPlacementAlignment;
		ur_size ImageDataPitchAlignment;
		GrafRayTracingProperties RayTracing;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafDeviceMemoryFlag
	{
		Undefined = 0,
		GpuLocal = (1 << 0),
		CpuVisible = (1 << 1)
	};
	typedef ur_uint GrafDeviceMemoryFlags;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafPresentMode
	{
		Immediate = 0,
		VerticalSync,
		Count
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/*enum GrafDeviceTypeFlags
	{
		GrafDeviceTypeFlag_Undefined = 0,
		GrafDeviceTypeFlag_Graphics = (1 << 0),
		GrafDeviceTypeFlag_Compute = (1 << 1),
		GrafDeviceTypeFlag_Transfer = (1 << 2)
	};*/

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafFenceState
	{
		Undefined = -1,
		Reset,
		Signaled
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafFormat
	{
		Unsupported = -1,
		Undefined = 0,
		R8_UNORM,
		R8_SNORM,
		R8_UINT,
		R8_SINT,
		R8G8_UINT,
		R8G8_SINT,
		R8G8B8_UNORM,
		R8G8B8_SNORM,
		R8G8B8_UINT,
		R8G8B8_SINT,
		R8G8B8_SRGB,
		B8G8R8_UNORM,
		B8G8R8_SNORM,
		B8G8R8_UINT,
		B8G8R8_SINT,
		B8G8R8_SRGB,
		R8G8B8A8_UNORM,
		R8G8B8A8_SNORM,
		R8G8B8A8_UINT,
		R8G8B8A8_SINT,
		R8G8B8A8_SRGB,
		B8G8R8A8_UNORM,
		B8G8R8A8_SNORM,
		B8G8R8A8_UINT,
		B8G8R8A8_SINT,
		B8G8R8A8_SRGB,
		R16_UINT,
		R16_SINT,
		R16_SFLOAT,
		R16G16_UINT,
		R16G16_SINT,
		R16G16_SFLOAT,
		R16G16B16_SFLOAT,
		R16G16B16A16_UNORM,
		R16G16B16A16_SNORM,
		R16G16B16A16_UINT,
		R16G16B16A16_SINT,
		R16G16B16A16_SFLOAT,
		R32_UINT,
		R32_SINT,
		R32_SFLOAT,
		R32G32_SFLOAT,
		R32G32B32_SFLOAT,
		R32G32B32A32_SFLOAT,
		R64_UINT,
		R64_SINT,
		R64_SFLOAT,
		D16_UNORM,
		X8_D24_UNORM_PACK32,
		D32_SFLOAT,
		S8_UINT,
		D16_UNORM_S8_UINT,
		D24_UNORM_S8_UINT,
		D32_SFLOAT_S8_UINT,
		BC1_RGB_UNORM_BLOCK,
		BC1_RGB_SRGB_BLOCK,
		BC1_RGBA_UNORM_BLOCK,
		BC1_RGBA_SRGB_BLOCK,
		BC3_UNORM_BLOCK,
		BC3_SRGB_BLOCK,
		Count
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafImageType
	{
		Undefined = 0,
		Tex1D,
		Tex2D,
		Tex3D,
		Count
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafImageUsageFlag
	{
		Undefined = 0,
		TransferSrc = (1 << 0),
		TransferDst = (1 << 1),
		ColorRenderTarget = (1 << 2),
		DepthStencilRenderTarget = (1 << 3),
		ShaderRead = (1 << 4),
		ShaderReadWrite = (1 << 5)
	};
	typedef ur_uint GrafImageUsageFlags;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafImageState
	{
		Current = -1,
		Undefined = 0,
		Common,
		TransferSrc,
		TransferDst,
		Present,
		ColorClear,
		ColorWrite,
		DepthStencilClear,
		DepthStencilWrite,
		DepthStencilRead,
		ShaderRead,
		ShaderReadWrite,
		ComputeRead,
		ComputeReadWrite,
		RayTracingRead,
		RayTracingReadWrite
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafImageDesc
	{
		GrafImageType Type;
		GrafFormat Format;
		ur_uint3 Size;
		ur_uint MipLevels;
		GrafImageUsageFlags Usage;
		GrafDeviceMemoryFlags MemoryType;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafImageSubresourceDesc
	{
		ur_uint32 BaseMipLevel;
		ur_uint32 LevelCount;
		ur_uint32 BaseArrayLayer;
		ur_uint32 LayerCount;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafBufferUsageFlag
	{
		Undefined = 0,
		TransferSrc = (1 << 0),
		TransferDst = (1 << 1),
		VertexBuffer = (1 << 2),
		IndexBuffer = (1 << 3),
		ConstantBuffer = (1 << 4),
		StorageBuffer = (1 << 5),
		ShaderDeviceAddress = (1 << 6),
		RayTracing = (1 << 7),
		AccelerationStructure = (1 << 8)
	};
	typedef ur_uint GrafBufferUsageFlags;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafBufferState
	{
		Current = -1,
		Undefined = 0,
		TransferSrc,
		TransferDst,
		VertexBuffer,
		IndexBuffer,
		ConstantBuffer,
		ShaderRead,
		ShaderReadWrite,
		ComputeConstantBuffer,
		ComputeRead,
		ComputeReadWrite,
		RayTracingConstantBuffer,
		RayTracingRead,
		RayTracingReadWrite
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafBufferDesc
	{
		GrafBufferUsageFlags Usage;
		GrafDeviceMemoryFlags MemoryType;
		ur_size SizeInBytes;
		ur_size ElementSize;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafStridedBufferRegionDesc
	{
		GrafBuffer* BufferPtr;
		ur_size Offset;
		ur_size Size;
		ur_size Stride;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafIndexType
	{
		Undefined = -1,
		None = 0,
		UINT16,
		UINT32
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafFilterType
	{
		Nearest,
		Linear
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafAddressMode
	{
		Wrap,
		Mirror,
		Clamp,
		Border,
		MirrorOnce
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafSamplerDesc
	{
		GrafFilterType FilterMin;
		GrafFilterType FilterMag;
		GrafFilterType FilterMip;
		GrafAddressMode AddressModeU;
		GrafAddressMode AddressModeV;
		GrafAddressMode AddressModeW;
		ur_bool AnisoFilterEanbled;
		ur_float AnisoFilterMax;
		ur_float MipLodBias;
		ur_float MipLodMin;
		ur_float MipLodMax;
		static const GrafSamplerDesc Default;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafClearValue
	{
		union
		{
			ur_float f32[4];
			ur_int i32[4];
			ur_uint u32[4];
		};
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafShaderStageFlag
	{
		Undefined = -1,
		Vertex = (0x1 << 0),
		Pixel = (0x1 << 1),
		Compute = (0x1 << 2),
		RayGen = (0x1 << 3),
		AnyHit = (0x1 << 4),
		ClosestHit = (0x1 << 5),
		Miss = (0x1 << 6),
		Intersection = (0x1 << 7),
		Callable = (0x1 << 8),
		Task = (0x1 << 9),
		Mesh = (0x1 << 10),
		AllGraphics = (Vertex | Pixel),
		AllRayTracing = (RayGen | AnyHit | ClosestHit | Miss | Intersection | Callable),
		All = ~(0)
	};
	typedef ur_uint GrafShaderStageFlags;
	typedef GrafShaderStageFlag GrafShaderType;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafRayTracingShaderGroupType
	{
		Undefined = -1,
		General,		// GrafShaderStageFlag: Raygen, Miss, Callable
		TrianglesHit,	// GrafShaderStageFlag: AnyHit, ClosestHit
		ProceduralHit	// GrafShaderStageFlag: Intersection, AnyHit, ClosestHit
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafRayTracingShaderGroupDesc
	{
		GrafRayTracingShaderGroupType Type;
		ur_uint32 GeneralShaderIdx;
		ur_uint32 AnyHitShaderIdx;
		ur_uint32 ClosestHitShaderIdx;
		ur_uint32 IntersectionShaderIdx;
		static const ur_uint32 UnusedShaderIdx = ur_uint32(-1);
		static const GrafRayTracingShaderGroupDesc Default;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafViewportDesc
	{
		ur_float X;
		ur_float Y;
		ur_float Width;
		ur_float Height;
		ur_float Near;
		ur_float Far;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafRenderPassDataOp
	{
		DontCare = 0,
		Clear,
		Load,
		Store
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafRenderPassImageDesc
	{
		GrafFormat Format;
		GrafImageState InitialState;
		GrafImageState FinalState;
		GrafRenderPassDataOp LoadOp;
		GrafRenderPassDataOp StoreOp;
		GrafRenderPassDataOp StencilLoadOp;
		GrafRenderPassDataOp StencilStoreOp;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafRenderPassDesc
	{
		GrafRenderPassImageDesc* Images;
		ur_uint ImageCount;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafPrimitiveTopology
	{
		PointList,
		LineList,
		LineStrip,
		TriangleList,
		TriangleStrip,
		TriangleFan
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafFrontFaceOrder
	{
		CounterClockwise,
		Clockwise
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafCullMode
	{
		None,
		Front,
		Back
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafVertexInputType
	{
		PerVertex,
		PerInstance
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafVertexElementDesc
	{
		GrafFormat Format;
		ur_uint Offset;
		const char* Semantic;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafVertexInputDesc
	{
		GrafVertexInputType InputType;
		ur_uint BindingIdx;
		ur_uint Stride;
		GrafVertexElementDesc* Elements;
		ur_uint ElementCount;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// note: binding ids of different register types can overlap (hlsl style)
	enum class GrafDescriptorRegisterType
	{
		Undefined = -1,
		ConstantBuffer,		// b
		Sampler,			// s
		ReadOnlyResource,	// t
		ReadWriteResource,	// u
		Count
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class GrafDescriptorType
	{
		Undefined = -1,
		ConstantBuffer,			// register type: ConstantBuffer
		Sampler,				// register type: Sampler
		Texture,				// register type: ReadOnlyResource
		Buffer,					// register type: ReadOnlyResource
		RWTexture,				// register type: ReadWriteResource
		RWBuffer,				// register type: ReadWriteResource
		AccelerationStructure,	// register type: ReadOnlyResource
		TextureDynamicArray,	// register type: ReadOnlyResource
		BufferDynamicArray,		// register type: ReadOnlyResource
		Count
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafDescriptorRangeDesc
	{
		GrafDescriptorType Type;
		ur_uint BindingOffset;
		ur_uint BindingCount;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafDescriptorTableLayoutDesc
	{
		GrafShaderStageFlags ShaderStageVisibility;
		GrafDescriptorRangeDesc* DescriptorRanges;
		ur_uint DescriptorRangeCount;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafCompareOp
	{
		Undefined = -1,
		Never = 0,
		Less,
		Equal,
		LessOrEqual,
		Greater,
		NotEqual,
		GreaterOrEqual,
		Always,
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafStencilOp
	{
		Undefined = -1,
		Keep = 0,
		Zero,
		Replace
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafStencilOpDesc
	{
		GrafStencilOp FailOp;
		GrafStencilOp PassOp;
		GrafStencilOp DepthFailOp;
		GrafCompareOp CompareOp;
		ur_uint32 CompareMask;
		ur_uint32 WriteMask;
		ur_uint32 Reference;
		static const GrafStencilOpDesc Default;
	};
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafColorChannelFlag
	{
		Red = (1 << 0),
		Green = (1 << 1),
		Blue = (1 << 2),
		Alpha = (1 << 3),
		All = (1 << 4) - 1
	};
	typedef ur_uint GrafColorChannelFlags;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafBlendOp
	{
		Undefined = -1,
		Add = 0,
		Subtract,
		ReverseSubtract,
		Min,
		Max
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafBlendFactor
	{
		Undefined = -1,
		Zero = 0,
		One,
		SrcColor,
		InvSrcColor,
		DstColor,
		InvDstColor,
		SrcAlpha,
		InvSrcAlpha,
		DstAlpha,
		InvDstAlpha
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafColorBlendOpDesc
	{
		GrafColorChannelFlags WriteMask;
		ur_bool BlendEnable;
		GrafBlendOp ColorOp;
		GrafBlendFactor SrcColorFactor;
		GrafBlendFactor DstColorFactor;
		GrafBlendOp AlphaOp;
		GrafBlendFactor SrcAlphaFactor;
		GrafBlendFactor DstAlphaFactor;
		static const GrafColorBlendOpDesc Default;
		static const GrafColorBlendOpDesc DefaultBlendEnable;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafAccelerationStructureType
	{
		Undefined = -1,
		TopLevel,
		BottomLevel
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafAccelerationStructureBuildFlag
	{
		AllowUpdate = (1 << 0),
		AllowCompaction = (1 << 1),
		PreferFastTrace = (1 << 2),
		PreferFastBuild = (1 << 3),
		MinimizeMemory = (1 << 4)
	};
	typedef ur_uint GrafAccelerationStructureBuildFlags;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafAccelerationStructureGeometryType
	{
		Undefined = -1,
		Triangles,
		AABBs,
		Instances
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafAccelerationStructureGeometryFlag
	{
		Opaque = (1 << 0),
		NoDuplicateAnyHit = (1 << 1)
	};
	typedef ur_uint GrafAccelerationStructureGeometryFlags;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafAccelerationStructureInstanceFlag
	{
		TriangleFacingCullDisable = (1 << 0),
		TriangleFrontCounterClockwise = (1 << 1),
		ForceOpaque = (1 << 2),
		ForceNoOpaque = (1 << 3)
	};
	typedef ur_uint GrafAccelerationStructureInstanceFlags;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafAccelerationStructureInstance
	{
		ur_float Transform[3][4];
		ur_uint32 Index : 24;
		ur_uint32 Mask : 8;
		ur_uint32 ShaderTableRecordOffset : 24;
		ur_uint32 Flags : 8;
		ur_uint64 AccelerationStructureHandle;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafAccelerationStructureTrianglesData
	{
		GrafFormat VertexFormat;
		ur_uint32 VertexStride;
		ur_uint32 VertexCount;
		ur_uint64 VerticesDeviceAddress;
		void* VerticesHostAddress;
		GrafIndexType IndexType;
		ur_uint64 IndicesDeviceAddress;
		void* IndicesHostAddress;
		ur_uint64 TransformsDeviceAddress;
		void* TransformsHostAddress;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafAccelerationStructureInstancesData
	{
		ur_bool IsPointersArray;
		ur_uint64 DeviceAddress;
		void* HostAddress;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafAccelerationStructureAabbsData
	{
		ur_uint32 Stride;
		ur_uint64 DeviceAddress;
		void* HostAddress;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafAccelerationStructureGeometryData
	{
		GrafAccelerationStructureGeometryType GeometryType;
		GrafAccelerationStructureGeometryFlags GeometryFlags;
		GrafAccelerationStructureTrianglesData* TrianglesData;
		GrafAccelerationStructureAabbsData* AabbsData;
		GrafAccelerationStructureInstancesData* InstancesData;
		ur_uint32 PrimitiveCount;
		ur_uint32 PrimitivesOffset;
		ur_uint32 FirstVertexIndex;
		ur_uint32 TransformsOffset;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafAccelerationStructureGeometryDesc
	{
		GrafAccelerationStructureGeometryType GeometryType;
		GrafFormat VertexFormat;
		ur_uint32 VertexStride;
		GrafIndexType IndexType;
		ur_uint32 PrimitiveCountMax;
		ur_uint32 VertexCountMax;
		ur_bool TransformsEnabled;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	typedef std::function<Result(ur_byte *mappedDataPtr)> GrafWriteCallback;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafSystem : public RealmEntity
	{
	public:

		GrafSystem(Realm &realm);

		virtual ~GrafSystem();

		virtual Result Initialize(Canvas *canvas);

		virtual Result CreateDevice(std::unique_ptr<GrafDevice>& grafDevice);

		virtual Result CreateCommandList(std::unique_ptr<GrafCommandList>& grafCommandList);

		virtual Result CreateFence(std::unique_ptr<GrafFence>& grafFence);

		virtual Result CreateCanvas(std::unique_ptr<GrafCanvas>& grafCanvas);

		virtual Result CreateImage(std::unique_ptr<GrafImage>& grafImage);

		virtual Result CreateImageSubresource(std::unique_ptr<GrafImageSubresource>& grafImageSubresource);

		virtual Result CreateBuffer(std::unique_ptr<GrafBuffer>& grafBuffer);

		virtual Result CreateSampler(std::unique_ptr<GrafSampler>& grafSampler);

		virtual Result CreateShader(std::unique_ptr<GrafShader>& grafShader);

		virtual Result CreateShaderLib(std::unique_ptr<GrafShaderLib>& grafShaderLib);

		virtual Result CreateRenderPass(std::unique_ptr<GrafRenderPass>& grafRenderPass);

		virtual Result CreateRenderTarget(std::unique_ptr<GrafRenderTarget>& grafRenderTarget);

		virtual Result CreateDescriptorTableLayout(std::unique_ptr<GrafDescriptorTableLayout>& grafDescriptorTableLayout);

		virtual Result CreateDescriptorTable(std::unique_ptr<GrafDescriptorTable>& grafDescriptorTable);

		virtual Result CreatePipeline(std::unique_ptr<GrafPipeline>& grafPipeline);

		virtual Result CreateComputePipeline(std::unique_ptr<GrafComputePipeline>& grafComputePipeline);

		virtual Result CreateRayTracingPipeline(std::unique_ptr<GrafRayTracingPipeline>& grafRayTracingPipeline);

		virtual Result CreateAccelerationStructure(std::unique_ptr<GrafAccelerationStructure>& grafAccelStruct);

		virtual ur_uint GetRecommendedDeviceId();

		virtual const char* GetShaderExtension() const;

		inline ur_uint GetPhysicalDeviceCount() const;

		inline const GrafPhysicalDeviceDesc* GetPhysicalDeviceDesc(ur_uint deviceId) const;

	protected:

		std::vector<GrafPhysicalDeviceDesc> grafPhysicalDeviceDesc;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafEntity : public RealmEntity
	{
	public:

		GrafEntity(GrafSystem &grafSystem);

		~GrafEntity();

		inline GrafSystem& GetGrafSystem();

	private:

		GrafSystem &grafSystem;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafDevice : public GrafEntity
	{
	public:

		GrafDevice(GrafSystem &grafSystem);

		~GrafDevice();

		virtual Result Initialize(ur_uint deviceId);

		virtual Result Record(GrafCommandList* grafCommandList);

		virtual Result Submit();

		virtual Result WaitIdle();

		inline ur_uint GetDeviceId();

		inline const GrafPhysicalDeviceDesc* GetPhysicalDeviceDesc();

	private:

		ur_uint deviceId;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafDeviceEntity : public GrafEntity
	{
	public:

		GrafDeviceEntity(GrafSystem &grafSystem);

		~GrafDeviceEntity();

		virtual Result Initialize(GrafDevice *grafDevice);

		inline GrafDevice* GetGrafDevice();

	private:

		GrafDevice *grafDevice;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafCommandList : public GrafDeviceEntity
	{
	public:

		GrafCommandList(GrafSystem &grafSystem);

		~GrafCommandList();

		virtual Result Initialize(GrafDevice *grafDevice);

		virtual Result Begin();

		virtual Result End();

		virtual Result Wait(ur_uint64 timeout = ur_uint64(-1));

		virtual Result InsertDebugLabel(const char* name, const ur_float4& color = ur_float4::Zero);

		virtual Result BeginDebugLabel(const char* name, const ur_float4& color = ur_float4::Zero);

		virtual Result EndDebugLabel();

		virtual Result BufferMemoryBarrier(GrafBuffer* grafBuffer, GrafBufferState srcState, GrafBufferState dstState);

		virtual Result ImageMemoryBarrier(GrafImage* grafImage, GrafImageState srcState, GrafImageState dstState);

		virtual Result ImageMemoryBarrier(GrafImageSubresource* grafImageSubresource, GrafImageState srcState, GrafImageState dstState);

		virtual Result SetFenceState(GrafFence* grafFence, GrafFenceState state);

		virtual Result ClearColorImage(GrafImage* grafImage, GrafClearValue clearValue);

		virtual Result ClearColorImage(GrafImageSubresource* grafImageSubresource, GrafClearValue clearValue);

		virtual Result ClearDepthStencilImage(GrafImage* grafImage, GrafClearValue clearValue);

		virtual Result ClearDepthStencilImage(GrafImageSubresource* grafImageSubresource, GrafClearValue clearValue);

		virtual Result SetViewport(const GrafViewportDesc& grafViewportDesc, ur_bool resetScissorsRect = false);

		virtual Result SetScissorsRect(const RectI& scissorsRect);

		virtual Result BeginRenderPass(GrafRenderPass* grafRenderPass, GrafRenderTarget* grafRenderTarget, GrafClearValue* rtClearValues = ur_null);

		virtual Result EndRenderPass();

		virtual Result BindPipeline(GrafPipeline* grafPipeline);

		virtual Result BindDescriptorTable(GrafDescriptorTable* descriptorTable, GrafPipeline* grafPipeline);

		virtual Result BindVertexBuffer(GrafBuffer* grafVertexBuffer, ur_uint bindingIdx, ur_size bufferOffset = 0);

		virtual Result BindIndexBuffer(GrafBuffer* grafIndexBuffer, GrafIndexType indexType, ur_size bufferOffset = 0);

		virtual Result Draw(ur_uint vertexCount, ur_uint instanceCount, ur_uint firstVertex, ur_uint firstInstance);

		virtual Result DrawIndexed(ur_uint indexCount, ur_uint instanceCount, ur_uint firstIndex, ur_uint firstVertex, ur_uint firstInstance);

		virtual Result Copy(GrafBuffer* srcBuffer, GrafBuffer* dstBuffer, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Copy(GrafBuffer* srcBuffer, GrafImage* dstImage, ur_size bufferOffset = 0, BoxI imageRegion = BoxI::Zero);

		virtual Result Copy(GrafBuffer* srcBuffer, GrafImageSubresource* dstImageSubresource, ur_size bufferOffset = 0, BoxI imageRegion = BoxI::Zero);

		virtual Result Copy(GrafImage* srcImage, GrafBuffer* dstBuffer, ur_size bufferOffset = 0, BoxI imageRegion = BoxI::Zero);

		virtual Result Copy(GrafImageSubresource* srcImageSubresource, GrafBuffer* dstBuffer, ur_size bufferOffset = 0, BoxI imageRegion = BoxI::Zero);

		virtual Result Copy(GrafImage* srcImage, GrafImage* dstImage, BoxI srcRegion = BoxI::Zero, BoxI dstRegion = BoxI::Zero);

		virtual Result Copy(GrafImageSubresource* srcImageSubresource, GrafImageSubresource* dstImageSubresource, BoxI srcRegion = BoxI::Zero, BoxI dstRegion = BoxI::Zero);

		virtual Result BindComputePipeline(GrafComputePipeline* grafPipeline);

		virtual Result BindComputeDescriptorTable(GrafDescriptorTable* descriptorTable, GrafComputePipeline* grafPipeline);

		virtual Result Dispatch(ur_uint groupCountX, ur_uint groupCountY, ur_uint groupCountZ);

		virtual Result BuildAccelerationStructure(GrafAccelerationStructure* dstStructrure, GrafAccelerationStructureGeometryData* geometryData, ur_uint geometryCount);

		virtual Result BindRayTracingPipeline(GrafRayTracingPipeline* grafPipeline);

		virtual Result BindRayTracingDescriptorTable(GrafDescriptorTable* descriptorTable, GrafRayTracingPipeline* grafPipeline);

		virtual Result DispatchRays(ur_uint width, ur_uint height, ur_uint depth,
			const GrafStridedBufferRegionDesc* rayGenShaderTable, const GrafStridedBufferRegionDesc* missShaderTable,
			const GrafStridedBufferRegionDesc* hitShaderTable, const GrafStridedBufferRegionDesc* callableShaderTable);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafFence : public GrafDeviceEntity
	{
	public:

		GrafFence(GrafSystem &grafSystem);

		~GrafFence();

		virtual Result Initialize(GrafDevice *grafDevice);

		virtual Result SetState(GrafFenceState state);

		virtual Result GetState(GrafFenceState& state);

		virtual Result WaitSignaled();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafCanvas : public GrafDeviceEntity
	{
	public:

		struct UR_DECL InitParams
		{
			GrafFormat Format;
			GrafPresentMode PresentMode;
			ur_uint SwapChainImageCount;
			static const InitParams Default;
		};

		GrafCanvas(GrafSystem &grafSystem);

		~GrafCanvas();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result Present();

		virtual GrafImage* GetCurrentImage();

		virtual GrafImage* GetSwapChainImage(ur_uint imageId);

		inline ur_uint GetSwapChainImageCount() const;

		inline ur_uint GetCurrentImageId() const;

	protected:

		ur_uint swapChainImageCount;
		ur_uint swapChainCurrentImageId;
		ur_uint swapChainSyncInterval;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafImage : public GrafDeviceEntity
	{
	public:

		struct UR_DECL InitParams
		{
			GrafImageDesc ImageDesc;
		};

		GrafImage(GrafSystem &grafSystem);

		~GrafImage();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result Write(const ur_byte* dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Write(GrafWriteCallback writeCallback, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Read(ur_byte*& dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		inline const GrafImageDesc& GetDesc() const;

		inline const GrafImageState& GetState() const;

	protected:

		GrafImageDesc imageDesc;
		GrafImageState imageState;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafImageSubresource : public GrafDeviceEntity
	{
	public:

		struct UR_DECL InitParams
		{
			GrafImage* Image;
			GrafImageSubresourceDesc SubresourceDesc;
		};

		GrafImageSubresource(GrafSystem &grafSystem);
		
		~GrafImageSubresource();
		
		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline const GrafImage* GetImage() const;

		inline const GrafImageSubresourceDesc& GetDesc() const;

		inline const GrafImageState& GetState() const;

	protected:

		GrafImage* image;
		GrafImageSubresourceDesc subresourceDesc;
		GrafImageState subresourceState;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafBuffer : public GrafDeviceEntity
	{
	public:

		struct UR_DECL InitParams
		{
			GrafBufferDesc BufferDesc;
		};

		GrafBuffer(GrafSystem &grafSystem);

		~GrafBuffer();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result Write(const ur_byte* dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Write(GrafWriteCallback writeCallback, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Read(ur_byte*& dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		inline const GrafBufferDesc& GetDesc() const;

		inline const GrafBufferState& GetState() const;

		inline ur_uint64 GetDeviceAddress() const;

	protected:

		GrafBufferDesc bufferDesc;
		GrafBufferState bufferState;
		ur_uint64 bufferDeviceAddress;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafSampler : public GrafDeviceEntity
	{
	public:

		struct UR_DECL InitParams
		{
			GrafSamplerDesc SamplerDesc;
		};

		GrafSampler(GrafSystem &grafSystem);

		~GrafSampler();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline const GrafSamplerDesc& GetDesc() const;

	protected:

		GrafSamplerDesc samplerDesc;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafShader : public GrafDeviceEntity
	{
	public:

		struct UR_DECL InitParams
		{
			GrafShaderType ShaderType;
			ur_byte* ByteCode;
			ur_size ByteCodeSize;
			const char* EntryPoint;
		};
		static const char* DefaultEntryPoint;

		GrafShader(GrafSystem &grafSystem);

		~GrafShader();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline const GrafShaderType& GetShaderType() const;

		inline const std::string& GetEntryPoint() const;

	protected:

		GrafShaderType shaderType;
		std::string entryPoint;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafShaderLib : public GrafDeviceEntity
	{
	public:

		struct UR_DECL EntryPoint
		{
			const char* Name;
			GrafShaderType Type;
		};

		struct UR_DECL InitParams
		{
			ur_byte* ByteCode;
			ur_size ByteCodeSize;
			EntryPoint* EntryPoints;
			ur_uint EntryPointCount;
		};

		GrafShaderLib(GrafSystem &grafSystem);

		~GrafShaderLib();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline GrafShader* GetShader(const ur_uint entryId) const;

	protected:

		std::vector<std::unique_ptr<GrafShader>> shaders;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafRenderPass : public GrafDeviceEntity
	{
	public:

		struct UR_DECL InitParams
		{
			GrafRenderPassDesc PassDesc;
		};

		GrafRenderPass(GrafSystem& grafSystem);

		~GrafRenderPass();

		virtual Result Initialize(GrafDevice* grafDevice, const InitParams& initParams);

		inline ur_size GetImageCount() const;

		inline const GrafRenderPassImageDesc& GetImageDesc(ur_size idx) const;

	protected:

		std::vector<GrafRenderPassImageDesc> renderPassImageDescs;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafRenderTarget : public GrafDeviceEntity
	{
	public:

		struct UR_DECL InitParams
		{
			GrafRenderPass* RenderPass;
			GrafImage** Images;
			ur_uint ImageCount;
		};

		GrafRenderTarget(GrafSystem &grafSystem);

		~GrafRenderTarget();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline GrafRenderPass* GetRenderPass() const;

		inline GrafImage* GetImage(ur_uint imageId) const;

		inline ur_uint GetImageCount() const;

	protected:

		GrafRenderPass* renderPass;
		std::vector<GrafImage*> images;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafDescriptorTableLayout : public GrafDeviceEntity
	{
	public:

		struct UR_DECL InitParams
		{
			GrafDescriptorTableLayoutDesc LayoutDesc;
		};

		GrafDescriptorTableLayout(GrafSystem &grafSystem);

		~GrafDescriptorTableLayout();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline const GrafDescriptorTableLayoutDesc& GetLayoutDesc() const;

	protected:

		GrafDescriptorTableLayoutDesc layoutDesc;
		std::vector<GrafDescriptorRangeDesc> descriptorRanges;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafDescriptorTable : public GrafDeviceEntity
	{
	public:

		struct UR_DECL InitParams
		{
			GrafDescriptorTableLayout* Layout;
		};

		GrafDescriptorTable(GrafSystem &grafSystem);

		~GrafDescriptorTable();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result SetConstantBuffer(ur_uint bindingIdx, GrafBuffer* buffer, ur_size bufferOfs = 0, ur_size bufferRange = 0);

		virtual Result SetSampledImage(ur_uint bindingIdx, GrafImage* image, GrafSampler* sampler);

		virtual Result SetSampledImage(ur_uint bindingIdx, GrafImageSubresource* imageSubresource, GrafSampler* sampler);

		virtual Result SetSampler(ur_uint bindingIdx, GrafSampler* sampler);

		virtual Result SetImage(ur_uint bindingIdx, GrafImage* image);

		virtual Result SetImage(ur_uint bindingIdx, GrafImageSubresource* imageSubresource);

		virtual Result SetImageArray(ur_uint bindingIdx, GrafImage** images, ur_uint imageCount);

		virtual Result SetBuffer(ur_uint bindingIdx, GrafBuffer* buffer);

		virtual Result SetBufferArray(ur_uint bindingIdx, GrafBuffer** buffers, ur_uint bufferCount);

		virtual Result SetRWImage(ur_uint bindingIdx, GrafImage* image);

		virtual Result SetRWImage(ur_uint bindingIdx, GrafImageSubresource* imageSubresource);

		virtual Result SetRWBuffer(ur_uint bindingIdx, GrafBuffer* buffer);

		virtual Result SetAccelerationStructure(ur_uint bindingIdx, GrafAccelerationStructure* accelerationStructure);


		inline GrafDescriptorTableLayout* GetLayout() const;

	protected:

		GrafDescriptorTableLayout* layout;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafPipeline : public GrafDeviceEntity
	{
	public:

		struct UR_DECL InitParams
		{
			GrafRenderPass* RenderPass;
			GrafShader** ShaderStages;
			ur_uint ShaderStageCount;
			GrafDescriptorTableLayout** DescriptorTableLayouts;
			ur_uint DescriptorTableLayoutCount;
			GrafVertexInputDesc* VertexInputDesc;
			ur_uint VertexInputCount;
			GrafPrimitiveTopology PrimitiveTopology;
			GrafFrontFaceOrder FrontFaceOrder;
			GrafCullMode CullMode;
			GrafViewportDesc ViewportDesc;
			ur_bool DepthTestEnable;
			ur_bool DepthWriteEnable;
			GrafCompareOp DepthCompareOp;
			ur_bool StencilTestEnable;
			GrafStencilOpDesc StencilFront;
			GrafStencilOpDesc StencilBack;
			GrafColorBlendOpDesc *ColorBlendOpDesc;
			ur_uint ColorBlendOpDescCount;
			static const InitParams Default;
		};

		GrafPipeline(GrafSystem &grafSystem);

		~GrafPipeline();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafComputePipeline : public GrafDeviceEntity
	{
	public:

		struct UR_DECL InitParams
		{
			GrafShader* ShaderStage;
			GrafDescriptorTableLayout** DescriptorTableLayouts;
			ur_uint DescriptorTableLayoutCount;
			static const InitParams Default;
		};

		GrafComputePipeline(GrafSystem &grafSystem);

		~GrafComputePipeline();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafRayTracingPipeline : public GrafDeviceEntity
	{
	public:

		struct UR_DECL InitParams
		{
			GrafShader** ShaderStages;
			ur_uint ShaderStageCount;
			GrafRayTracingShaderGroupDesc* ShaderGroups;
			ur_uint ShaderGroupCount;
			GrafDescriptorTableLayout** DescriptorTableLayouts;
			ur_uint DescriptorTableLayoutCount;
			ur_uint MaxRecursionDepth;
			static const InitParams Default;
		};

		GrafRayTracingPipeline(GrafSystem &grafSystem);

		~GrafRayTracingPipeline();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result GetShaderGroupHandles(ur_uint firstGroup, ur_uint groupCount, ur_size dstBufferSize, ur_byte* dstBufferPtr);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafAccelerationStructure : public GrafDeviceEntity
	{
	public:

		struct UR_DECL InitParams
		{
			GrafAccelerationStructureType StructureType;
			GrafAccelerationStructureBuildFlags BuildFlags;
			GrafAccelerationStructureGeometryDesc* Geometry;
			ur_uint GeometryCount;
		};

		GrafAccelerationStructure(GrafSystem &grafSystem);

		~GrafAccelerationStructure();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);


		inline GrafAccelerationStructureType GetStructureType() const;

		inline GrafAccelerationStructureBuildFlags GetStructureBuildFlags() const;

		inline ur_uint64 GetDeviceAddress() const;

	protected:

		GrafAccelerationStructureType structureType;
		GrafAccelerationStructureBuildFlags structureBuildFlags;
		ur_uint64 structureDeviceAddress;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafUtils
	{
	public:

		static inline ur_bool IsDepthStencilFormat(GrafFormat grafFormat);
		static inline ur_bool IsDescriptorTypeWithDynamicIndexing(GrafDescriptorType grafDescriptorType);

		class UR_DECL ScopedDebugLabel
		{
		public:
			ScopedDebugLabel(GrafCommandList* cmdList, const char* name, const ur_float4& color = ur_float4::Zero);
			~ScopedDebugLabel();
		private:
			GrafCommandList* cmdList;
		};

		struct ImageData
		{
			GrafImageDesc Desc;
			ur_uint RowPitch;
			std::vector<std::unique_ptr<GrafBuffer>> MipBuffers;
		};

		enum class MeshVertexElementFlag : ur_uint
		{
			Position = (1 << 0),
			Normal = (1 << 1),
			Tangent = (1 << 2),
			Color = (1 << 3),
			TexCoord = (1 << 4),
			All = ~ur_uint(0)
		};
		typedef ur_uint MeshVertexElementFlags;
		typedef MeshVertexElementFlag MeshVertexElementType;

		struct MeshVertexElementDesc
		{
			MeshVertexElementType Type;
			GrafFormat Format;
		};

		struct MeshMaterialDesc
		{
			ur_float3 BaseColor;
			ur_float3 EmissiveColor;
			ur_float Roughness;
			ur_float Metallic;
			ur_float Reflectance;
			ur_float ClearCoat;
			ur_float ClearCoatRoughness;
			ur_float Anisotropy;
			ur_float3 AnisotropyDirection;
			ur_float3 SheenColor;

			std::string ColorTexName;
			std::string MaskTexName;
			std::string NormalTexName;
			std::string DisplacementTexName;
			std::string RoughnessTexName;
			std::string MetallicTexName;
			std::string EmissiveTexName;

			static const MeshMaterialDesc Default;
		};

		struct MeshSurfaceData
		{
			ur_uint PrimitivesOffset; // offset in indices if available, otherwise in vertices
			ur_uint PrimtivesCount; // number of triangles
			ur_uint MaterialID;
		};

		struct MeshData
		{
			std::vector<MeshVertexElementDesc> VertexElements;
			MeshVertexElementFlags VertexElementFlags;
			GrafIndexType IndexType;
			std::vector<ur_byte> Vertices;
			std::vector<ur_byte> Indices;
			std::vector<MeshMaterialDesc> Materials;
			std::vector<MeshSurfaceData> Surfaces;
		};

		struct ModelData
		{
			std::vector<std::unique_ptr<MeshData>> Meshes;
		};

		static Result CreateShaderFromFile(GrafDevice& grafDevice, const std::string& resName, GrafShaderType shaderType, std::unique_ptr<GrafShader>& grafShader);

		static Result CreateShaderFromFile(GrafDevice& grafDevice, const std::string& resName, const std::string& entryPoint, GrafShaderType shaderType, std::unique_ptr<GrafShader>& grafShader);

		static Result CreateShaderLibFromFile(GrafDevice& grafDevice, const std::string& resName, const GrafShaderLib::EntryPoint* entryPoints, ur_uint entryPointCount, std::unique_ptr<GrafShaderLib>& grafShaderLib);

		static Result LoadImageFromFile(GrafDevice& grafDevice, const std::string& resName, ImageData& outputImageData);

		static Result LoadModelFromFile(GrafDevice& grafDevice, const std::string& resName, ModelData& modelData, MeshVertexElementFlags vertexMask = (ur_uint)MeshVertexElementFlag::All);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	#define PROFILER_ENABLED 1
	class UR_DECL Profiler
	{
	public:

		struct UR_DECL ScopedMarker
		{
		public:

			ScopedMarker(Log* log = ur_null, const char* message = ur_null);
			~ScopedMarker();

		private:

			Log* log;
			const char* message;
		};

		static const void Begin();

		static const ur_uint64 End(Log* log = ur_null, const char* message = ur_null);
	};

	#if (PROFILER_ENABLED)
	#define PROFILE_LINE(code_line, log) Profiler::Begin(); code_line; Profiler::End(log, #code_line);
	#define PROFILE(code_line, log, message) Profiler::Begin(); code_line; Profiler::End(log, message);
	#else
	#define PROFILE_LINE(code_line, log) code_line
	#define PROFILE(code_line, log, message) code_line
	#endif

} // end namespace UnlimRealms

#include "GrafSystem.inline.h"