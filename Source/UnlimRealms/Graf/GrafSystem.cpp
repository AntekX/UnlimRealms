///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "GrafSystem.h"
#include "Sys/Log.h"
#include "Sys/Storage.h"
#include "3rdParty/ResIL/include/IL/il.h"

namespace UnlimRealms
{

	const GrafSamplerDesc GrafSamplerDesc::Default = {
		GrafFilterType::Linear, // FilterMin
		GrafFilterType::Linear, // FilterMag
		GrafFilterType::Linear, // FilterMip
		GrafAddressMode::Wrap, // AddressModeU
		GrafAddressMode::Wrap, // AddressModeV
		GrafAddressMode::Wrap, // AddressModeW
		false, // AnisoFilterEanbled
		1.0f, // AnisoFilterMax
		0.0f, // MipLodBias
		0.0f, // MipLodMin
		128.0f, // MipLodMax
	};

	const GrafRayTracingShaderGroupDesc GrafRayTracingShaderGroupDesc::Default = {
		GrafRayTracingShaderGroupType::Undefined, // GrafRayTracingShaderGroupType
		UnusedShaderIdx, // GeneralShaderIdx;
		UnusedShaderIdx, // AnyHitShaderIdx;
		UnusedShaderIdx, // ClosestHitShaderIdx;
		UnusedShaderIdx, // IntersectionShaderIdx;
	};

	const GrafStencilOpDesc GrafStencilOpDesc::Default = {
		GrafStencilOp::Keep, // FailOp
		GrafStencilOp::Keep, // PassOp
		GrafStencilOp::Keep, // DepthFailOp
		GrafCompareOp::Never, // CompareOp
		0x0, // CompareMask
		0x0, // WriteMask
		0x0 // Reference
	};

	const GrafColorBlendOpDesc GrafColorBlendOpDesc::Default = {
		GrafColorChannelFlags(GrafColorChannelFlag::All),
		false, // BlendEnable
		GrafBlendOp::Add, // GrafBlendOp
		GrafBlendFactor::SrcAlpha, // GrafBlendFactor
		GrafBlendFactor::InvSrcAlpha, // GrafBlendFactor
		GrafBlendOp::Add, // GrafBlendOp
		GrafBlendFactor::One, // GrafBlendFactor
		GrafBlendFactor::Zero, // GrafBlendFactor
	};

	const GrafColorBlendOpDesc GrafColorBlendOpDesc::DefaultBlendEnable = {
		GrafColorChannelFlags(GrafColorChannelFlag::All),
		true, // BlendEnable
		GrafBlendOp::Add, // GrafBlendOp
		GrafBlendFactor::SrcAlpha, // GrafBlendFactor
		GrafBlendFactor::InvSrcAlpha, // GrafBlendFactor
		GrafBlendOp::Add, // GrafBlendOp
		GrafBlendFactor::One, // GrafBlendFactor
		GrafBlendFactor::Zero, // GrafBlendFactor
	};

	GrafSystem::GrafSystem(Realm &realm) :
		RealmEntity(realm)
	{
		LogNote("GrafSystem: created");
	}

	GrafSystem::~GrafSystem()
	{
		LogNote("GrafSystem: destroyed");
	}

	Result GrafSystem::Initialize(Canvas *canvas)
	{
		return Result(NotImplemented);
	}

	Result GrafSystem::CreateDevice(std::unique_ptr<GrafDevice>& grafDevice)
	{
		return Result(NotImplemented);
	}

	Result GrafSystem::CreateCommandList(std::unique_ptr<GrafCommandList>& grafCommandList)
	{
		return Result(NotImplemented);
	}

	Result GrafSystem::CreateFence(std::unique_ptr<GrafFence>& grafFence)
	{
		return Result(NotImplemented);
	}

	Result GrafSystem::CreateCanvas(std::unique_ptr<GrafCanvas>& grafCanvas)
	{
		return Result(NotImplemented);
	}

	Result GrafSystem::CreateImage(std::unique_ptr<GrafImage>& grafImage)
	{
		return Result(NotImplemented);
	}

	Result GrafSystem::CreateBuffer(std::unique_ptr<GrafBuffer>& grafBuffer)
	{
		return Result(NotImplemented);
	}

	Result GrafSystem::CreateSampler(std::unique_ptr<GrafSampler>& grafSampler)
	{
		return Result(NotImplemented);
	}

	Result GrafSystem::CreateShader(std::unique_ptr<GrafShader>& grafShader)
	{
		return Result(NotImplemented);
	}

	Result GrafSystem::CreateRenderPass(std::unique_ptr<GrafRenderPass>& grafRenderPass)
	{
		return Result(NotImplemented);
	}

	Result GrafSystem::CreateRenderTarget(std::unique_ptr<GrafRenderTarget>& grafRenderTarget)
	{
		return Result(NotImplemented);
	}

	Result GrafSystem::CreateDescriptorTableLayout(std::unique_ptr<GrafDescriptorTableLayout>& grafDescriptorTableLayout)
	{
		return Result(NotImplemented);
	}

	Result GrafSystem::CreateDescriptorTable(std::unique_ptr<GrafDescriptorTable>& grafDescriptorTable)
	{
		return Result(NotImplemented);
	}

	Result GrafSystem::CreatePipeline(std::unique_ptr<GrafPipeline>& grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafSystem::CreateRayTracingPipeline(std::unique_ptr<GrafRayTracingPipeline>& grafRayTracingPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafSystem::CreateAccelerationStructure(std::unique_ptr<GrafAccelerationStructure>& grafAccelStruct)
	{
		return Result(NotImplemented);
	}

	ur_uint GrafSystem::GetRecommendedDeviceId()
	{
		ur_uint recommendedDeviceId = ur_uint(-1);

		GrafPhysicalDeviceDesc bestDeviceDesc = {};
		for (ur_uint deviceId = 0; deviceId < (ur_uint)grafPhysicalDeviceDesc.size(); ++deviceId)
		{
			const GrafPhysicalDeviceDesc& deviceDesc = grafPhysicalDeviceDesc[deviceId];
			if (deviceDesc.DedicatedVideoMemory > bestDeviceDesc.DedicatedVideoMemory)
			{
				bestDeviceDesc.DedicatedVideoMemory = deviceDesc.DedicatedVideoMemory;
				recommendedDeviceId = deviceId;
			}
		}

		return recommendedDeviceId;
	}

	GrafEntity::GrafEntity(GrafSystem &grafSystem) :
		RealmEntity(grafSystem.GetRealm()),
		grafSystem(grafSystem)
	{
	}

	GrafEntity::~GrafEntity()
	{
	}

	GrafDevice::GrafDevice(GrafSystem &grafSystem) :
		GrafEntity(grafSystem)
	{
	}

	GrafDevice::~GrafDevice()
	{
	}

	Result GrafDevice::Initialize(ur_uint deviceId)
	{
		this->deviceId = deviceId;
		return Result(NotImplemented);
	}

	Result GrafDevice::Record(GrafCommandList* grafCommandList)
	{
		return Result(NotImplemented);
	}

	Result GrafDevice::Submit()
	{
		return Result(NotImplemented);
	}

	Result GrafDevice::WaitIdle()
	{
		return Result(NotImplemented);
	}

	GrafDeviceEntity::GrafDeviceEntity(GrafSystem &grafSystem) :
		GrafEntity(grafSystem),
		grafDevice(ur_null)
	{
	}

	GrafDeviceEntity::~GrafDeviceEntity()
	{
	}

	Result GrafDeviceEntity::Initialize(GrafDevice *grafDevice)
	{
		this->grafDevice = grafDevice;
		return Result(NotImplemented);
	}

	GrafCommandList::GrafCommandList(GrafSystem &grafSystem) : 
		GrafDeviceEntity(grafSystem)
	{
	}

	GrafCommandList::~GrafCommandList()
	{
	}

	Result GrafCommandList::Initialize(GrafDevice *grafDevice)
	{
		GrafDeviceEntity::Initialize(grafDevice);
		return Result(NotImplemented);
	}

	Result GrafCommandList::Begin()
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::End()
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::Wait(ur_uint64 timeout)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::ImageMemoryBarrier(GrafImage* grafImage, GrafImageState srcState, GrafImageState dstState)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::BufferMemoryBarrier(GrafBuffer* grafBuffer, GrafBufferState srcState, GrafBufferState dstState)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::SetFenceState(GrafFence* grafFence, GrafFenceState state)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::ClearColorImage(GrafImage* grafImage, GrafClearValue clearValue)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::SetViewport(const GrafViewportDesc& grafViewportDesc, ur_bool resetScissorsRect)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::SetScissorsRect(const RectI& scissorsRect)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::BeginRenderPass(GrafRenderPass* grafRenderPass, GrafRenderTarget* grafRenderTarget, GrafClearValue* rtClearValues)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::EndRenderPass()
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::BindPipeline(GrafPipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::BindDescriptorTable(GrafDescriptorTable* descriptorTable, GrafPipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::BindVertexBuffer(GrafBuffer* grafVertexBuffer, ur_uint bindingIdx, ur_size bufferOffset)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::BindIndexBuffer(GrafBuffer* grafIndexBuffer, GrafIndexType indexType, ur_size bufferOffset)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::Draw(ur_uint vertexCount, ur_uint instanceCount, ur_uint firstVertex, ur_uint firstInstance)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::DrawIndexed(ur_uint indexCount, ur_uint instanceCount, ur_uint firstIndex, ur_uint firstVertex, ur_uint firstInstance)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::Copy(GrafBuffer* srcBuffer, GrafBuffer* dstBuffer, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::Copy(GrafBuffer* srcBuffer, GrafImage* dstImage, ur_size bufferOffset, BoxI imageRegion)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::Copy(GrafImage* srcImage, GrafBuffer* dstBuffer, ur_size bufferOffset, BoxI imageRegion)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::Copy(GrafImage* srcImage, GrafImage* dstImage, BoxI srcRegion, BoxI dstRegion)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::BindComputePipeline(GrafPipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::BindComputeDescriptorTable(GrafDescriptorTable* descriptorTable, GrafPipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::Dispatch(ur_uint groupCountX, ur_uint groupCountY, ur_uint groupCountZ)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::BuildAccelerationStructure(GrafAccelerationStructure* dstStructrure, GrafAccelerationStructureGeometryData* geometryData, ur_uint geometryCount)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::BindRayTracingPipeline(GrafRayTracingPipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::BindRayTracingDescriptorTable(GrafDescriptorTable* descriptorTable, GrafRayTracingPipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::DispatchRays(ur_uint width, ur_uint height, ur_uint depth,
		const GrafStridedBufferRegionDesc* rayGenShaderTable, const GrafStridedBufferRegionDesc* missShaderTable,
		const GrafStridedBufferRegionDesc* hitShaderTable, const GrafStridedBufferRegionDesc* callableShaderTable)
	{
		return Result(NotImplemented);
	}

	GrafFence::GrafFence(GrafSystem &grafSystem) :
		GrafDeviceEntity(grafSystem)
	{
	}

	GrafFence::~GrafFence()
	{
	}

	Result GrafFence::Initialize(GrafDevice *grafDevice)
	{
		GrafDeviceEntity::Initialize(grafDevice);
		return Result(NotImplemented);
	}

	Result GrafFence::SetState(GrafFenceState state)
	{
		return Result(NotImplemented);
	}

	Result GrafFence::GetState(GrafFenceState& state)
	{
		return Result(NotImplemented);
	}

	Result GrafFence::WaitSignaled()
	{
		return Result(NotImplemented);
	}

	GrafCanvas::GrafCanvas(GrafSystem &grafSystem) :
		GrafDeviceEntity(grafSystem)
	{
		this->swapChainImageCount = 0;
		this->swapChainCurrentImageId = 0;
	}

	GrafCanvas::~GrafCanvas()
	{
	}

	const GrafCanvas::InitParams GrafCanvas::InitParams::Default = {
		GrafFormat::B8G8R8A8_UNORM, GrafPresentMode::VerticalSync, 3
	};

	Result GrafCanvas::Initialize(GrafDevice* grafDevice, const InitParams& initParams)
	{
		GrafDeviceEntity::Initialize(grafDevice);
		this->swapChainImageCount = initParams.SwapChainImageCount;
		return Result(NotImplemented);
	}

	Result GrafCanvas::Present()
	{
		return Result(NotImplemented);
	}

	GrafImage* GrafCanvas::GetCurrentImage()
	{
		return ur_null;
	}

	GrafImage* GrafCanvas::GetSwapChainImage(ur_uint imageId)
	{
		return ur_null;
	}

	GrafImage::GrafImage(GrafSystem &grafSystem) :
		GrafDeviceEntity(grafSystem)
	{
		this->imageState = GrafImageState::Undefined;
	}

	GrafImage::~GrafImage()
	{
	}

	Result GrafImage::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		GrafDeviceEntity::Initialize(grafDevice);
		this->imageDesc = initParams.ImageDesc;
		return Result(NotImplemented);
	}

	Result GrafImage::Write(const ur_byte* dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	Result GrafImage::Write(GrafWriteCallback writeCallback, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	Result GrafImage::Read(ur_byte*& dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	GrafBuffer::GrafBuffer(GrafSystem &grafSystem) :
		GrafDeviceEntity(grafSystem)
	{
		this->bufferState = GrafBufferState::Undefined;
	}

	GrafBuffer::~GrafBuffer()
	{
	}

	Result GrafBuffer::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		GrafDeviceEntity::Initialize(grafDevice);
		this->bufferDesc = initParams.BufferDesc;
		this->bufferDeviceAddress = ur_uint64(-1);
		return Result(NotImplemented);
	}

	Result GrafBuffer::Write(const ur_byte* dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	Result GrafBuffer::Write(GrafWriteCallback writeCallback, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	Result GrafBuffer::Read(ur_byte*& dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	GrafSampler::GrafSampler(GrafSystem &grafSystem) :
		GrafDeviceEntity(grafSystem)
	{
	}

	GrafSampler::~GrafSampler()
	{
	}

	Result GrafSampler::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		GrafDeviceEntity::Initialize(grafDevice);
		this->samplerDesc = initParams.SamplerDesc;
		return Result(NotImplemented);
	}

	const char* GrafShader::DefaultEntryPoint = "main";

	GrafShader::GrafShader(GrafSystem &grafSystem) :
		GrafDeviceEntity(grafSystem)
	{
	}

	GrafShader::~GrafShader()
	{
	}

	Result GrafShader::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		GrafDeviceEntity::Initialize(grafDevice);
		this->shaderType = initParams.ShaderType;
		this->entryPoint = (initParams.EntryPoint ? initParams.EntryPoint : DefaultEntryPoint);
		return Result(NotImplemented);
	}

	GrafRenderTarget::GrafRenderTarget(GrafSystem &grafSystem) :
		GrafDeviceEntity(grafSystem)
	{
		this->renderPass = ur_null;
	}

	GrafRenderTarget::~GrafRenderTarget()
	{
	}

	Result GrafRenderTarget::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		GrafDeviceEntity::Initialize(grafDevice);
		this->renderPass = initParams.RenderPass;
		this->images.resize(initParams.ImageCount);
		memcpy(this->images.data(), initParams.Images, initParams.ImageCount * sizeof(GrafImage*));
		return Result(NotImplemented);
	}

	GrafDescriptorTableLayout::GrafDescriptorTableLayout(GrafSystem &grafSystem) :
		GrafDeviceEntity(grafSystem)
	{
		this->layoutDesc = {};
	}

	GrafDescriptorTableLayout::~GrafDescriptorTableLayout()
	{
	}

	Result GrafDescriptorTableLayout::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		GrafDeviceEntity::Initialize(grafDevice);
	
		// keep local copy of layout descriptopn
		this->descriptorRanges.resize(initParams.LayoutDesc.DescriptorRangeCount);
		memcpy(this->descriptorRanges.data(), initParams.LayoutDesc.DescriptorRanges, this->descriptorRanges.size() * sizeof(GrafDescriptorRangeDesc));
		this->layoutDesc = initParams.LayoutDesc;
		this->layoutDesc.DescriptorRanges = this->descriptorRanges.data();

		return Result(NotImplemented);
	}

	GrafDescriptorTable::GrafDescriptorTable(GrafSystem &grafSystem) :
		GrafDeviceEntity(grafSystem)
	{
		this->layout = ur_null;
	}

	GrafDescriptorTable::~GrafDescriptorTable()
	{
	}

	Result GrafDescriptorTable::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		GrafDeviceEntity::Initialize(grafDevice);
		this->layout = initParams.Layout;
		return Result(NotImplemented);
	}

	Result GrafDescriptorTable::SetConstantBuffer(ur_uint bindingIdx, GrafBuffer* buffer, ur_size bufferOfs, ur_size bufferRange)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTable::SetSampledImage(ur_uint bindingIdx, GrafImage* image, GrafSampler* sampler)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTable::SetSampler(ur_uint bindingIdx, GrafSampler* sampler)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTable::SetImage(ur_uint bindingIdx, GrafImage* image)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTable::SetBuffer(ur_uint bindingIdx, GrafBuffer* buffer)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTable::SetRWImage(ur_uint bindingIdx, GrafImage* image)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTable::SetRWBuffer(ur_uint bindingIdx, GrafBuffer* buffer)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTable::SetAccelerationStructure(ur_uint bindingIdx, GrafAccelerationStructure* accelerationStructure)
	{
		return Result(NotImplemented);
	}

	static GrafColorBlendOpDesc GrafPipelineDefaultColorBlendOpDesc = GrafColorBlendOpDesc::Default;
	const GrafPipeline::InitParams GrafPipeline::InitParams::Default = {
		ur_null, // RenderPass
		ur_null, // GrafShader**
		0, // ShaderStageCount
		ur_null, // GrafDescriptorTableLayout**
		0, // DescriptorTableLayoutCount;
		ur_null, // GrafVertexInputDesc
		0, // VertexInputCount
		GrafPrimitiveTopology::TriangleList, // GrafPrimitiveTopology
		GrafFrontFaceOrder::Clockwise, // GrafFrontFaceOrder
		GrafCullMode::None, // GrafCullMode
		{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f }, //GrafViewportDesc
		false, // DepthTestEnable
		false, // DepthWriteEnable
		GrafCompareOp::LessOrEqual, // DepthCompareOp
		false, // StencilTestEnable
		GrafStencilOpDesc::Default, // GrafStencilOpDesc
		GrafStencilOpDesc::Default, // GrafStencilOpDesc
		&GrafPipelineDefaultColorBlendOpDesc, // GrafColorBlendOpDesc
		1 // ColorBlendOpDescCount
	};

	GrafPipeline::GrafPipeline(GrafSystem &grafSystem) :
		GrafDeviceEntity(grafSystem)
	{
	}

	GrafPipeline::~GrafPipeline()
	{
	}

	Result GrafPipeline::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		GrafDeviceEntity::Initialize(grafDevice);
		return Result(NotImplemented);
	}

	const GrafRayTracingPipeline::InitParams GrafRayTracingPipeline::InitParams::Default = {
		ur_null, // GrafShader**
		0, // ShaderStageCount;
		ur_null, // GrafRayTracingShaderGroupDesc*
		0, // ShaderGroupCount
		ur_null, // GrafDescriptorTableLayout**
		0, // DescriptorTableLayoutCount;
		1 // MaxRecursionDepth
	};

	GrafRayTracingPipeline::GrafRayTracingPipeline(GrafSystem &grafSystem) :
		GrafDeviceEntity(grafSystem)
	{
	}

	GrafRayTracingPipeline::~GrafRayTracingPipeline()
	{
	}

	Result GrafRayTracingPipeline::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		GrafDeviceEntity::Initialize(grafDevice);
		return Result(NotImplemented);
	}

	Result GrafRayTracingPipeline::GetShaderGroupHandles(ur_uint firstGroup, ur_uint groupCount, ur_size dstBufferSize, ur_byte* dstBufferPtr)
	{
		return Result(NotImplemented);
	}

	GrafRenderPass::GrafRenderPass(GrafSystem& grafSystem) :
		GrafDeviceEntity(grafSystem)
	{
	}

	GrafRenderPass::~GrafRenderPass()
	{
	}

	Result GrafRenderPass::Initialize(GrafDevice* grafDevice, const InitParams& initParams)
	{
		GrafDeviceEntity::Initialize(grafDevice);
		this->renderPassImageDescs.resize(initParams.PassDesc.ImageCount);
		memcpy(this->renderPassImageDescs.data(), initParams.PassDesc.Images, sizeof(GrafRenderPassImageDesc) * initParams.PassDesc.ImageCount);
		return Result(NotImplemented);
	}

	GrafAccelerationStructure::GrafAccelerationStructure(GrafSystem &grafSystem) :
		GrafDeviceEntity(grafSystem)
	{
		this->structureType = GrafAccelerationStructureType::Undefined;
		this->structureBuildFlags = GrafAccelerationStructureBuildFlags(0);
		this->structureDeviceAddress = 0;
	}

	GrafAccelerationStructure::~GrafAccelerationStructure()
	{
	}

	Result GrafAccelerationStructure::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		GrafDeviceEntity::Initialize(grafDevice);
		this->structureType = initParams.StructureType;
		this->structureBuildFlags = initParams.BuildFlags;
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Result GrafUtils::LoadImageFromFile(GrafDevice& grafDevice, const std::string& resName, ImageData& outputImageData)
	{
		Realm& realm = grafDevice.GetRealm();
		GrafSystem& grafSystem = grafDevice.GetGrafSystem();

		ILconst_string ilResName;
		#ifdef _UNICODE
		// convert name
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> strConverter;
		std::wstring resNameW = strConverter.from_bytes(resName);
		ilResName = resNameW.c_str();
		#else
		ilResName = resName.c_str();
		#endif

		// load

		ILuint imageId;
		ilInit();
		ilGenImages(1, &imageId);
		ilBindImage(imageId);
		ilSetInteger(IL_KEEP_DXTC_DATA, IL_TRUE);
		ilSetInteger(IL_DECOMPRESS_DXTC, IL_FALSE);
		if (ilLoadImage(ilResName) == IL_FALSE)
			return LogResult(Failure, realm.GetLog(), Log::Error, "LoadImageFromFile: failed to load image " + resName);

		ILint ilWidth, ilHeight, ilMips, ilFormat, ilDXTFormat, ilBpp, ilBpc;
		ilGetImageInteger(IL_IMAGE_WIDTH, &ilWidth);
		ilGetImageInteger(IL_IMAGE_HEIGHT, &ilHeight);
		ilGetImageInteger(IL_IMAGE_FORMAT, &ilFormat);
		ilGetImageInteger(IL_IMAGE_BPP, &ilBpp);
		ilGetImageInteger(IL_IMAGE_BPC, &ilBpc);
		ilGetImageInteger(IL_NUM_MIPMAPS, &ilMips);
		ilGetImageInteger(IL_DXTC_DATA_FORMAT, &ilDXTFormat);

		// fill destination image description

		outputImageData.Desc.Type = GrafImageType::Tex2D;
		outputImageData.Desc.Format = GrafFormat::R8G8B8A8_UNORM;
		outputImageData.Desc.Size.x = (ur_uint)ilWidth;
		outputImageData.Desc.Size.y = (ur_uint)ilHeight;
		outputImageData.Desc.Size.z = 1;
		outputImageData.Desc.MipLevels = (ur_uint)ilMips + 1;
		outputImageData.Desc.Usage = (ur_uint)GrafImageUsageFlag::TransferDst | (ur_uint)GrafImageUsageFlag::ShaderInput;
		outputImageData.Desc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::GpuLocal;
		outputImageData.RowPitch = (ur_uint)ilWidth * ilBpp * ilBpc;
		switch (ilDXTFormat)
		{
		case IL_DXT1:
			outputImageData.Desc.Format = GrafFormat::BC1_RGBA_UNORM_BLOCK;
			outputImageData.RowPitch /= 6;
			break;
		case IL_DXT5: outputImageData.Desc.Format = GrafFormat::BC3_UNORM_BLOCK;
			outputImageData.RowPitch /= 4;
			break;
		}

		// copy mip levels into cpu visible buffers

		Result res = Result(Success);
		ur_uint mipRowPitch = outputImageData.RowPitch;
		ur_uint mipHeight = outputImageData.Desc.Size.y;
		outputImageData.MipBuffers.reserve(ilMips + 1);
		for (ur_uint imip = 0; imip < outputImageData.Desc.MipLevels; ++imip)
		{
			std::unique_ptr<GrafBuffer> mipBuffer;
			res = grafSystem.CreateBuffer(mipBuffer);
			if (Failed(res))
				break;

			GrafBufferDesc mipBufferDesc;
			mipBufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::TransferSrc;
			mipBufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
			mipBufferDesc.SizeInBytes = mipRowPitch * mipHeight;
			res = mipBuffer->Initialize(&grafDevice, { mipBufferDesc });
			if (Failed(res))
				break;

			ur_byte* mipDataPtr = ur_null;
			if (ilDXTFormat != IL_DXT_NO_COMP)
			{
				mipDataPtr = (ur_byte*)(0 == imip ? ilGetDXTCData() : ilGetMipDXTCData(imip - 1));
			}
			else
			{
				mipDataPtr = (ur_byte*)(0 == imip ? ilGetData() : ilGetMipData(imip - 1));
			}
		
			res = mipBuffer->Write(mipDataPtr);
			if (Failed(res))
				break;

			outputImageData.MipBuffers.push_back(std::move(mipBuffer));

			mipRowPitch /= 2;
			mipHeight /= 2;
		}

		// release IL resources

		ilDeleteImages(1, &imageId);

		if (Failed(res))
		{
			outputImageData.MipBuffers.clear();
			return LogResult(Failure, realm.GetLog(), Log::Error, "LoadImageFromFile: failed to load image " + resName);
		}
		return res;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Result GrafUtils::CreateShaderFromFile(GrafDevice& grafDevice, const std::string& resName, GrafShaderType shaderType, std::unique_ptr<GrafShader>& grafShader)
	{
		return  GrafUtils::CreateShaderFromFile(grafDevice, resName, GrafShader::DefaultEntryPoint, shaderType, grafShader);
	}

	Result GrafUtils::CreateShaderFromFile(GrafDevice& grafDevice, const std::string& resName, const std::string& entryPoint, GrafShaderType shaderType, std::unique_ptr<GrafShader>& grafShader)
	{
		Realm& realm = grafDevice.GetRealm();
		GrafSystem& grafSystem = grafDevice.GetGrafSystem();

		// load

		std::unique_ptr<ur_byte[]> shaderBuffer;
		ur_size shaderBufferSize = 0;
		{
			std::unique_ptr<File> file;
			Result res = realm.GetStorage().Open(file, resName, ur_uint(StorageAccess::Read) | ur_uint(StorageAccess::Binary));
			if (Succeeded(res))
			{
				shaderBufferSize = file->GetSize();
				shaderBuffer.reset(new ur_byte[shaderBufferSize]);
				res &= file->Read(shaderBufferSize, shaderBuffer.get());
			}
			if (Failed(res))
			{
				return LogResult(Failure, realm.GetLog(), Log::Error, "CreateShaderFromFile: failed to load shader " + resName);
			}
		}

		// initialize graf shader object

		Result grafRes = grafSystem.CreateShader(grafShader);
		if (Failed(grafRes))
		{
			return LogResult(Failure, realm.GetLog(), Log::Error, "CreateShaderFromFile: failed to create shader " + resName);
		}

		grafRes = grafShader->Initialize(&grafDevice, { shaderType, shaderBuffer.get(), shaderBufferSize, entryPoint.c_str() });
		if (Failed(grafRes))
		{
			return LogResult(Failure, realm.GetLog(), Log::Error, "CreateShaderFromFile: failed to create shader " + resName);
		}

		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	static thread_local ClockTime ProfilerThreadTimerBegin;
	static thread_local ClockTime ProfilerThreadTimerEnd;

	const void Profiler::Begin()
	{
	#if (PROFILER_ENABLED)
		ProfilerThreadTimerBegin = Clock::now();
	#endif
	}

	const ur_uint64 Profiler::End(Log* log, const char* message)
	{
	#if (PROFILER_ENABLED)
		ProfilerThreadTimerEnd = Clock::now();
		auto deltaTime = ClockDeltaAs<std::chrono::microseconds>(ProfilerThreadTimerEnd - ProfilerThreadTimerBegin);
		if (log != ur_null)
		{
			log->WriteLine(std::string(message ? message : "") + std::string(" Profiler time (ns) = ") + std::to_string(deltaTime.count()));
		}
		return (ur_uint64)deltaTime.count();
	#else
		return 0;
	#endif
	}

	Profiler::ScopedMarker::ScopedMarker(Log* log, const char* message)
	{
		this->log = log;
		this->message = message;
		Profiler::Begin();
	}

	Profiler::ScopedMarker::~ScopedMarker()
	{	
		Profiler::End(this->log, this->message);
	}

} // end namespace UnlimRealms