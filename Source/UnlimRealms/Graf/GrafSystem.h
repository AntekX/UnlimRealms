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

namespace UnlimRealms
{

	// forward declarations
	class GrafSystem;
	class GrafDevice;
	class GrafCommandList;
	class GrafFence;
	class GrafCanvas;
	class GrafImage;
	class GrafBuffer;
	class GrafSampler;
	class GrafShader;
	class GrafRenderPass;
	class GrafRenderTarget;
	class GrafDescriptorTableLayout;
	class GrafDescriptorTable;
	class GrafPipeline;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafPhysicalDeviceDesc
	{
		std::string Description;
		ur_uint VendorId;
		ur_uint DeviceId;
		ur_size DedicatedVideoMemory;
		ur_size DedicatedSystemMemory;
		ur_size SharedSystemMemory;
		ur_size ConstantBufferOffsetAlignment;
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
		R32_SFLOAT,
		R32G32_SFLOAT,
		R32G32B32_SFLOAT,
		R32G32B32A32_SFLOAT,
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
		ShaderInput = (1 << 4)
	};
	typedef ur_uint GrafImageUsageFlags;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafImageState
	{
		Current = -1,
		Undefined = 0,
		Common,
		ColorWrite,
		DepthStencilWrite,
		DepthStencilRead,
		ShaderRead,
		TransferSrc,
		TransferDst,
		Present
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
	enum class UR_DECL GrafBufferUsageFlag
	{
		Undefined = 0,
		TransferSrc = (1 << 0),
		TransferDst = (1 << 1),
		VertexBuffer = (1 << 2),
		IndexBuffer = (1 << 3),
		ConstantBuffer = (1 << 4)
	};
	typedef ur_uint GrafBufferUsageFlags;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GrafBufferDesc
	{
		GrafBufferUsageFlags Usage;
		GrafDeviceMemoryFlags MemoryType;
		ur_size SizeInBytes;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafIndexType
	{
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
	enum class UR_DECL GrafShaderType
	{
		Undefined = -1,
		Vertex,
		Pixel,
		Compute,
		Count
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GrafShaderStageFlag
	{
		Undefined = -1,
		Vertex = (0x1 << 0),
		Pixel = (0x1 << 1),
		Compute = (0x1 << 2),
		All = (Vertex | Pixel | Compute)
	};
	typedef ur_uint GrafShaderStageFlags;

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
	// note: binding ids from different spaces can overlap
	enum class GrafDescriptorSpaceType
	{
		Undefined = -1,
		Buffer,
		Sampler,
		Texture,
		RWResource,
		Count
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class GrafDescriptorType
	{
		Undefined = -1,
		ConstantBuffer,	// space: Buffer
		Sampler,		// space: Sampler
		Texture,		// space: Texture
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

		virtual Result CreateBuffer(std::unique_ptr<GrafBuffer>& grafBuffer);

		virtual Result CreateSampler(std::unique_ptr<GrafSampler>& grafSampler);

		virtual Result CreateShader(std::unique_ptr<GrafShader>& grafShader);

		virtual Result CreateRenderPass(std::unique_ptr<GrafRenderPass>& grafRenderPass);

		virtual Result CreateRenderTarget(std::unique_ptr<GrafRenderTarget>& grafRenderTarget);

		virtual Result CreateDescriptorTableLayout(std::unique_ptr<GrafDescriptorTableLayout>& grafDescriptorTableLayout);

		virtual Result CreateDescriptorTable(std::unique_ptr<GrafDescriptorTable>& grafDescriptorTable);

		virtual Result CreatePipeline(std::unique_ptr<GrafPipeline>& grafPipeline);

		virtual ur_uint GetRecommendedDeviceId();

		inline ur_uint GetPhysicalDeviceCount();

		inline const GrafPhysicalDeviceDesc* GetPhysicalDeviceDesc(ur_uint deviceId);

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

		virtual Result Submit(GrafCommandList* grafCommandList);

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

		virtual Result ImageMemoryBarrier(GrafImage* grafImage, GrafImageState srcState, GrafImageState dstState);

		virtual Result SetFenceState(GrafFence* grafFence, GrafFenceState state);

		virtual Result ClearColorImage(GrafImage* grafImage, GrafClearValue clearValue);

		virtual Result SetViewport(const GrafViewportDesc& grafViewportDesc, ur_bool resetScissorsRect = false);

		virtual Result SetScissorsRect(const RectI& scissorsRect);

		virtual Result BeginRenderPass(GrafRenderPass* grafRenderPass, GrafRenderTarget* grafRenderTarget, GrafClearValue* rtClearValues = ur_null);

		virtual Result EndRenderPass();

		virtual Result BindPipeline(GrafPipeline* grafPipeline);

		virtual Result BindDescriptorTable(GrafDescriptorTable* descriptorTable, GrafPipeline* grafPipeline);

		virtual Result BindVertexBuffer(GrafBuffer* grafVertexBuffer, ur_uint bindingIdx);

		virtual Result BindIndexBuffer(GrafBuffer* grafIndexBuffer, GrafIndexType indexType);

		virtual Result Draw(ur_uint vertexCount, ur_uint instanceCount, ur_uint firstVertex, ur_uint firstInstance);

		virtual Result DrawIndexed(ur_uint indexCount, ur_uint instanceCount, ur_uint firstIndex, ur_uint firstVertex, ur_uint firstInstance);

		virtual Result Copy(GrafBuffer* srcBuffer, GrafBuffer* dstBuffer, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Copy(GrafBuffer* srcBuffer, GrafImage* dstImage, ur_size bufferOffset = 0, BoxI imageRegion = BoxI::Zero);

		virtual Result Copy(GrafImage* srcImage, GrafBuffer* dstBuffer, ur_size bufferOffset = 0, BoxI imageRegion = BoxI::Zero);

		virtual Result Copy(GrafImage* srcImage, GrafImage* dstImage, BoxI srcRegion = BoxI::Zero, BoxI dstRegion = BoxI::Zero);
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

		virtual Result Write(ur_byte* dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Write(GrafWriteCallback writeCallback, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Read(ur_byte*& dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		inline const GrafImageDesc& GetDesc() const;

		inline const GrafImageState& GetState() const;

	protected:

		GrafImageDesc imageDesc;
		GrafImageState imageState;
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

		virtual Result Write(ur_byte* dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Write(GrafWriteCallback writeCallback, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Read(ur_byte*& dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		inline const GrafBufferDesc& GetDesc() const;

	protected:

		GrafBufferDesc bufferDesc;
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

		virtual Result SetSampler(ur_uint bindingIdx, GrafSampler* sampler);

		virtual Result SetImage(ur_uint bindingIdx, GrafImage* image);


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
			GrafViewportDesc ViewportDesc;
			ur_bool DepthTestEnable;
			ur_bool DepthWriteEnable;
			GrafCompareOp DepthCompareOp;
			ur_bool StencilTestEnable;
			ur_bool BlendEnable;
			static const InitParams Default;
		};

		GrafPipeline(GrafSystem &grafSystem);

		~GrafPipeline();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafUtils
	{
	public:

		static inline ur_bool IsDepthStencilFormat(GrafFormat grafFormat);

		struct ImageData
		{
			GrafImageDesc Desc;
			ur_uint RowPitch;
			std::vector<std::unique_ptr<GrafBuffer>> MipBuffers;
		};

		static Result LoadImageFromFile(GrafDevice& grafDevice, const std::string& resName, ImageData& outputImageData);

		static Result CreateShaderFromFile(GrafDevice& grafDevice, const std::string& resName, GrafShaderType shaderType, std::unique_ptr<GrafShader>& grafShader);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	#define PROFILER_ENABLED 0
	class UR_DECL Profiler
	{
	public:

		struct ScopedMarker
		{
		public:

			ScopedMarker(Log* log = ur_null, const char* message = ur_null);
			~ScopedMarker();

		private:

			Log* log;
			const char* message;
		};

		static inline const void Begin();

		static inline const ur_uint64 End(Log* log = ur_null, const char* message = ur_null);
	};

	#if (PROFILER_ENABLED)
	#define PROFILE_LINE(code_line, log) { Profiler ::ScopedMarker marker(log, #code_line); code_line; }
	#define PROFILE(code_line, log, message) { Profiler ::ScopedMarker marker(log, message); code_line; }
	#else
	#define PROFILE_LINE(code_line, log) code_line
	#define PROFILE(code_line, log, message) code_line
	#endif

} // end namespace UnlimRealms

#include "GrafSystem.inline.h"