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
#define TINYOBJLOADER_IMPLEMENTATION
#include "3rdParty/TinyObjLoader/tiny_obj_loader.h"

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

	Result GrafSystem::CreateShaderLib(std::unique_ptr<GrafShaderLib>& grafShaderLib)
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

	Result GrafSystem::CreateComputePipeline(std::unique_ptr<GrafComputePipeline>& grafComputePipeline)
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

	Result GrafCommandList::BindComputePipeline(GrafComputePipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::BindComputeDescriptorTable(GrafDescriptorTable* descriptorTable, GrafComputePipeline* grafPipeline)
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

	GrafShaderLib::GrafShaderLib(GrafSystem &grafSystem) :
		GrafDeviceEntity(grafSystem)
	{
	}

	GrafShaderLib::~GrafShaderLib()
	{
	}

	Result GrafShaderLib::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		GrafDeviceEntity::Initialize(grafDevice);
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

	const GrafComputePipeline::InitParams GrafComputePipeline::InitParams::Default = {
		ur_null, // GrafShader*
		ur_null, // GrafDescriptorTableLayout**
		0 // DescriptorTableLayoutCount;
	};

	GrafComputePipeline::GrafComputePipeline(GrafSystem &grafSystem) :
		GrafDeviceEntity(grafSystem)
	{
	}

	GrafComputePipeline::~GrafComputePipeline()
	{
	}

	Result GrafComputePipeline::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
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

	Result GrafUtils::CreateShaderFromFile(GrafDevice& grafDevice, const std::string& resName, GrafShaderType shaderType, std::unique_ptr<GrafShader>& grafShader)
	{
		return GrafUtils::CreateShaderFromFile(grafDevice, resName, GrafShader::DefaultEntryPoint, shaderType, grafShader);
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

	Result GrafUtils::CreateShaderLibFromFile(GrafDevice& grafDevice, const std::string& resName, const GrafShaderLib::EntryPoint* entryPoints, ur_uint entryPointCount, std::unique_ptr<GrafShaderLib>& grafShaderLib)
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
				return LogResult(Failure, realm.GetLog(), Log::Error, "CreateShaderLibFromFile: failed to load shader lib " + resName);
			}
		}

		// initialize graf shader library

		Result grafRes = grafSystem.CreateShaderLib(grafShaderLib);
		if (Failed(grafRes))
		{
			return LogResult(Failure, realm.GetLog(), Log::Error, "CreateShaderLibFromFile: failed to create shader lib " + resName);
		}

		GrafShaderLib::InitParams shaderLibParams = {};
		shaderLibParams.ByteCode = shaderBuffer.get();
		shaderLibParams.ByteCodeSize = shaderBufferSize;
		shaderLibParams.EntryPoints = const_cast<GrafShaderLib::EntryPoint*>(entryPoints);
		shaderLibParams.EntryPointCount = entryPointCount;
		grafRes = grafShaderLib->Initialize(&grafDevice, shaderLibParams);
		if (Failed(grafRes))
		{
			return LogResult(Failure, realm.GetLog(), Log::Error, "CreateShaderLibFromFile: failed to initialize shader lib " + resName);
		}

		return Result(Success);
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

	const GrafUtils::MeshMaterialDesc GrafUtils::MeshMaterialDesc::Default = {
		{ 1.0f, 1.0f, 1.0f }, // BaseColor
		{ 0.0f, 0.0f, 0.0f }, // EmissiveColor
		1.0f, // Roughness
		0.0f, // Metallic
		0.5f, // Reflectance
		0.0f, // ClearCoat
		0.0f, // ClearCoatRoughness
		0.0f, // Anisotropy
		{ 1.0f, 0.0f, 0.0f }, // AnisotropyDirection
		{ 1.0f, 1.0f, 1.0f }, // SheenColor
	};

	Result GrafUtils::LoadModelFromFile(GrafDevice& grafDevice, const std::string& resName, ModelData& modelData, MeshVertexElementFlags vertexMask)
	{
		Realm& realm = grafDevice.GetRealm();
		GrafSystem& grafSystem = grafDevice.GetGrafSystem();

		ur_size delimPos = resName.rfind("/");
		if (std::string::npos == delimPos) delimPos = resName.rfind("\\");
		std::string resPath = resName.substr(0, delimPos);

		tinyobj::attrib_t attribs;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warnStr;
		std::string errorStr;

		ur_bool loadRes = tinyobj::LoadObj(&attribs, &shapes, &materials, &warnStr, &errorStr, resName.c_str(), resPath.c_str());
		//if (!warnStr.empty())
		//	LogResult(Failure, realm.GetLog(), Log::Warning, "CreateModelFromFile: failed while loading " + resName + ", " + warnStr);
		if (!errorStr.empty())
			LogResult(Failure, realm.GetLog(), Log::Error, "CreateModelFromFile: failed while loading " + resName + ", " + errorStr);
		if (!loadRes)
			return Result(Failure);

		std::vector<MeshVertexElementDesc> modelVertexElements;
		MeshVertexElementFlags usedVertexElementFlags = 0;
		ur_size vertexStride = 0;
		ur_size vertexPositionOfs = 0;
		if (!attribs.vertices.empty() && (vertexMask & ur_uint(MeshVertexElementFlag::Position)))
		{
			usedVertexElementFlags |= ur_uint(MeshVertexElementFlag::Position);
			MeshVertexElementDesc vertexElement = { MeshVertexElementType::Position, GrafFormat::R32G32B32_SFLOAT };
			modelVertexElements.emplace_back(vertexElement);
			vertexPositionOfs = vertexStride;
			vertexStride += 12;
		}
		ur_size vertexNormalOfs = 0;
		if (!attribs.normals.empty() && (vertexMask & ur_uint(MeshVertexElementFlag::Normal)))
		{
			usedVertexElementFlags |= ur_uint(MeshVertexElementFlag::Normal);
			MeshVertexElementDesc vertexElement = { MeshVertexElementType::Normal, GrafFormat::R32G32B32_SFLOAT };
			modelVertexElements.emplace_back(vertexElement);
			vertexNormalOfs = vertexStride;
			vertexStride += 12;
		}
		ur_size vertexColorOfs = 0;
		if (!attribs.colors.empty() && (vertexMask & ur_uint(MeshVertexElementFlag::Color)))
		{
			usedVertexElementFlags |= ur_uint(MeshVertexElementFlag::Color);
			MeshVertexElementDesc vertexElement = { MeshVertexElementType::Color, GrafFormat::R32G32B32_SFLOAT };
			modelVertexElements.emplace_back(vertexElement);
			vertexColorOfs = vertexStride;
			vertexStride += 12;
		}
		ur_size vertexTexcoordOfs = 0;
		if (!attribs.texcoords.empty() && (vertexMask & ur_uint(MeshVertexElementFlag::TexCoord)))
		{
			usedVertexElementFlags |= ur_uint(MeshVertexElementFlag::TexCoord);
			MeshVertexElementDesc vertexElement = { MeshVertexElementType::TexCoord, GrafFormat::R32G32_SFLOAT };
			modelVertexElements.emplace_back(vertexElement);
			vertexTexcoordOfs = vertexStride;
			vertexStride += 8;
		}

		// parse meta data
		// TEMP: use only the last shape with "lod" in it's name (for ray tracing test purpose),
		// otherwise all shapes are merged into one mesh
		tinyobj::shape_t *shapeToUse = ur_null;
		for (auto& shape : shapes)
		{
			//shape.name. += shape.mesh.indices.size();
			std::transform(shape.name.begin(), shape.name.end(), shape.name.begin(), std::tolower);
			ur_size lodSpos = shape.name.rfind("lod");
			if (lodSpos != std::string::npos)
			{
				shapeToUse = &shape;
			}
		}

		// here we assume that all mesh shapes describe one mesh
		// note: shape name parameter cane be used to group shapes into different meshes (e.g. LoDs)
		modelData.Meshes.emplace_back(new MeshData());
		MeshData& meshData = *modelData.Meshes.back();
		meshData.VertexElements = modelVertexElements; // all meshes share model's vertex format
		meshData.VertexElementFlags = usedVertexElementFlags;
		meshData.IndexType = GrafIndexType::UINT32;
		ur_size indexStride = sizeof(ur_uint32);

		// init materials
		meshData.Materials.resize(materials.size() + 1);
		meshData.Materials.back() = MeshMaterialDesc::Default;
		ur_size defaultMaterialID = meshData.Materials.size() - 1;
		for (ur_size im = 0; im < materials.size(); ++im)
		{
			const tinyobj::material_t& srcMat = materials[im];
			MeshMaterialDesc& dstMat = meshData.Materials[im];
			dstMat = MeshMaterialDesc::Default;
			dstMat.BaseColor.x = (ur_float)srcMat.diffuse[0];
			dstMat.BaseColor.y = (ur_float)srcMat.diffuse[1];
			dstMat.BaseColor.z = (ur_float)srcMat.diffuse[2];
			dstMat.EmissiveColor.x = (ur_float)srcMat.emission[0];
			dstMat.EmissiveColor.y = (ur_float)srcMat.emission[1];
			dstMat.EmissiveColor.z = (ur_float)srcMat.emission[2];
			dstMat.Roughness = (ur_float)srcMat.roughness;
			dstMat.Metallic = (ur_float)srcMat.metallic;
			dstMat.Reflectance = (ur_float)std::max(std::max(srcMat.specular[0], srcMat.specular[1]), srcMat.specular[2]);
			dstMat.ClearCoat = (ur_float)srcMat.clearcoat_thickness;
			dstMat.ClearCoatRoughness = (ur_float)srcMat.clearcoat_roughness;
			dstMat.Anisotropy = (ur_float)srcMat.anisotropy;
			dstMat.AnisotropyDirection.x = (ur_float)std::cosf(srcMat.anisotropy_rotation);
			dstMat.AnisotropyDirection.y = (ur_float)std::sinf(srcMat.anisotropy_rotation);
			dstMat.SheenColor.x = (ur_float)srcMat.sheen;
			dstMat.SheenColor.y = (ur_float)srcMat.sheen;
			dstMat.SheenColor.z = (ur_float)srcMat.sheen;

			dstMat.ColorTexName = srcMat.diffuse_texname;
			dstMat.NormalTexName = srcMat.normal_texname;
			dstMat.DisplacementTexName = srcMat.displacement_texname;
			dstMat.RoughnessTexName = srcMat.roughness_texname;
			dstMat.MetallicTexName = srcMat.metallic_texname;
			dstMat.EmissiveTexName = srcMat.emissive_texname;
		}

		// precalculate per vertex smooth normals
		ur_bool normalsSmoothingEnabled = false;
		std::vector<ur_float3> smoothNormals;
		if (!attribs.normals.empty() && (vertexMask & ur_uint(MeshVertexElementFlag::Normal)) && normalsSmoothingEnabled)
		{
			smoothNormals.resize(attribs.vertices.size() / 3);
			memset(smoothNormals.data(), 0x00, smoothNormals.size() * sizeof(ur_float3));
			for (ur_size ishape = 0; ishape < shapes.size(); ++ishape)
			{
				const tinyobj::shape_t& shape = shapes[ishape];
				if (shapeToUse && shapeToUse != &shape)
					continue;
				if (shape.mesh.indices.empty())
					continue; // mesh shapes accepted only
				ur_size nidx;
				ur_size indicesCount = shape.mesh.indices.size();
				for (ur_size ii = 0; ii < indicesCount; ++ii)
				{
					ur_float3& sn = smoothNormals[shape.mesh.indices[ii].vertex_index];
					nidx = shape.mesh.indices[ii].normal_index * 3;
					sn.x += attribs.normals[nidx + 0];
					sn.y += attribs.normals[nidx + 1];
					sn.z += attribs.normals[nidx + 2];
					sn.Normalize();
				}
			}
		}

		// TODO: optimize shared attributes usage, pack indices
		// currectly all primtives vertices are unique to simplify loading logic
		ur_size totalMeshIndicesCount = 0;
		for (auto& shape : shapes)
		{
			if (shapeToUse && shapeToUse != &shape)
				continue;
			totalMeshIndicesCount += shape.mesh.indices.size();
		}
		meshData.Indices.resize(totalMeshIndicesCount * indexStride);
		meshData.Vertices.resize(totalMeshIndicesCount * vertexStride);
		ur_size indicesOffset = 0;
		ur_size verticesOffset = 0;

		for (ur_size ishape = 0; ishape < shapes.size(); ++ishape)
		{
			const tinyobj::shape_t& shape = shapes[ishape];
			if (shapeToUse && shapeToUse != &shape)
				continue;
			if (shape.mesh.indices.empty())
				continue; // mesh shapes accepted only

			meshData.Surfaces.emplace_back(MeshSurfaceData());
			MeshSurfaceData& surfaceData = meshData.Surfaces.back();
			ur_int shapeMaterialID = shape.mesh.material_ids[0];
			surfaceData.MaterialID = (ur_uint)(shapeMaterialID >= 0 && shapeMaterialID < (ur_int)meshData.Materials.size() ? shapeMaterialID : defaultMaterialID); // consider all faces in one shape share the same material
			surfaceData.PrimtivesOffset = (ur_uint)indicesOffset;
			surfaceData.PrimtivesCount = (ur_uint)shape.mesh.indices.size() / 3;

			ur_byte* indicesPtr = meshData.Indices.data() + indicesOffset * indexStride;
			ur_uint32* typedIndexPtr = (ur_uint32*)indicesPtr;
			ur_size indicesCount = shape.mesh.indices.size();
			for (ur_size ii = 0; ii < indicesCount; ++ii)
			{
				*typedIndexPtr++ = ur_uint32(ii + indicesOffset);
			}

			ur_byte* verticesPtr = meshData.Vertices.data() + verticesOffset * vertexStride;
			if (!attribs.vertices.empty() && (vertexMask & ur_uint(MeshVertexElementFlag::Position)))
			{
				ur_byte* vertexElementPtr = (verticesPtr + vertexPositionOfs);
				ur_float* typedElementPtr;
				ur_size attribIdx;
				for (ur_size ii = 0; ii < indicesCount; ++ii)
				{
					attribIdx = shape.mesh.indices[ii].vertex_index * 3;
					typedElementPtr = (ur_float*)vertexElementPtr;
					typedElementPtr[0] = attribs.vertices[attribIdx + 0];
					typedElementPtr[1] = attribs.vertices[attribIdx + 1];
					typedElementPtr[2] = attribs.vertices[attribIdx + 2];
					vertexElementPtr += vertexStride;
				}
			}
			if (!attribs.normals.empty() && (vertexMask & ur_uint(MeshVertexElementFlag::Normal)))
			{
				ur_byte* vertexElementPtr = (verticesPtr + vertexNormalOfs);
				ur_float* typedElementPtr;
				if (smoothNormals.empty())
				{
					ur_size attribIdx;
					for (ur_size ii = 0; ii < indicesCount; ++ii)
					{
						attribIdx = shape.mesh.indices[ii].normal_index * 3;
						typedElementPtr = (ur_float*)vertexElementPtr;
						typedElementPtr[0] = attribs.normals[attribIdx + 0];
						typedElementPtr[1] = attribs.normals[attribIdx + 1];
						typedElementPtr[2] = attribs.normals[attribIdx + 2];
						vertexElementPtr += vertexStride;
					}
				}
				else
				{
					for (ur_size ii = 0; ii < indicesCount; ++ii)
					{
						ur_float3& sn = smoothNormals[shape.mesh.indices[ii].vertex_index];
						typedElementPtr = (ur_float*)vertexElementPtr;
						typedElementPtr[0] = sn.x;
						typedElementPtr[1] = sn.y;
						typedElementPtr[2] = sn.z;
						vertexElementPtr += vertexStride;
					}
				}
			}
			if (!attribs.vertices.empty() && (vertexMask & ur_uint(MeshVertexElementFlag::Color)))
			{
				ur_byte* vertexElementPtr = (verticesPtr + vertexColorOfs);
				ur_float* typedElementPtr;
				ur_size attribIdx;
				for (ur_size ii = 0; ii < indicesCount; ++ii)
				{
					attribIdx = shape.mesh.indices[ii].vertex_index * 3;
					typedElementPtr = (ur_float*)vertexElementPtr;
					typedElementPtr[0] = attribs.colors[attribIdx + 0];
					typedElementPtr[1] = attribs.colors[attribIdx + 1];
					typedElementPtr[2] = attribs.colors[attribIdx + 2];
					vertexElementPtr += vertexStride;
				}
			}
			if (!attribs.normals.empty() && (vertexMask & ur_uint(MeshVertexElementFlag::TexCoord)))
			{
				ur_byte* vertexElementPtr = (verticesPtr + vertexTexcoordOfs);
				ur_float* typedElementPtr;
				ur_size attribIdx;
				for (ur_size ii = 0; ii < indicesCount; ++ii)
				{
					attribIdx = shape.mesh.indices[ii].texcoord_index * 2;
					typedElementPtr = (ur_float*)vertexElementPtr;
					typedElementPtr[0] = attribs.texcoords[attribIdx + 0];
					typedElementPtr[1] = attribs.texcoords[attribIdx + 1];
					vertexElementPtr += vertexStride;
				}
			}

			indicesOffset += indicesCount;
			verticesOffset += indicesCount;
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