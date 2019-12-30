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

	Result GrafDevice::Submit(GrafCommandList* grafCommandList)
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

	Result GrafCommandList::BeginRenderPass(GrafRenderPass* grafRenderPass, GrafRenderTarget* grafRenderTarget)
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

	Result GrafCommandList::BindVertexBuffer(GrafBuffer* grafVertexBuffer, ur_uint bindingIdx)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandList::BindIndexBuffer(GrafBuffer* grafIndexBuffer, GrafIndexType indexType)
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

	Result GrafImage::Write(ur_byte* dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
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
	}

	GrafBuffer::~GrafBuffer()
	{
	}

	Result GrafBuffer::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		GrafDeviceEntity::Initialize(grafDevice);
		this->bufferDesc = initParams.BufferDesc;
		return Result(NotImplemented);
	}

	Result GrafBuffer::Write(ur_byte* dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
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

	const GrafPipeline::InitParams GrafPipeline::InitParams::Default = {
		ur_null, // RenderPass
		ur_null, // GrafShader**
		0, // ShaderStageCount
		ur_null, // GrafDescriptorTableLayout**
		0, // DescriptorTableLayoutCount;
		ur_null, // GrafVertexInputDesc
		0, // VertexInputCount
		GrafPrimitiveTopology::TriangleList, // GrafPrimitiveTopology
		{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f }, //GrafViewportDesc
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

	GrafRenderPass::GrafRenderPass(GrafSystem& grafSystem) :
		GrafDeviceEntity(grafSystem)
	{
	}

	GrafRenderPass::~GrafRenderPass()
	{
	}

	Result GrafRenderPass::Initialize(GrafDevice* grafDevice)
	{
		GrafDeviceEntity::Initialize(grafDevice);
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

		// copy mip levels inot cpu visible buffers

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
		
		grafRes = grafShader->Initialize(&grafDevice, { shaderType, shaderBuffer.get(), shaderBufferSize, GrafShader::DefaultEntryPoint });
		if (Failed(grafRes))
		{
			return LogResult(Failure, realm.GetLog(), Log::Error, "CreateShaderFromFile: failed to create shader " + resName);
		}

		return Result(Success);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	static thread_local ClockTime ProfilerThreadTimerBegin;
	static thread_local ClockTime ProfilerThreadTimerEnd;

	inline const void Profiler::Begin()
	{
	#if (PROFILER_ENABLED)
		ProfilerThreadTimerBegin = Clock::now();
	#endif
	}

	inline const ur_uint64 Profiler::End(Log* log, const char* message)
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Sys/Canvas.h"
#include "ImguiRender/ImguiRender.h"

namespace UnlimRealms
{

	const GrafRenderer::InitParams GrafRenderer::InitParams::Default = {
		GrafRenderer::InitParams::RecommendedDeviceId,
		GrafCanvas::InitParams::Default,
		20 * (1 << 20), // DynamicUploadBufferSize
		20 * (1 << 20), // DynamicConstantBufferSize
	};
	
	const ur_uint GrafRenderer::InitParams::RecommendedDeviceId = ur_uint(-1);

	GrafRenderer::GrafRenderer(Realm &realm) :
		RealmEntity(realm)
	{
	}

	GrafRenderer::~GrafRenderer()
	{
		this->Deinitialize();
	}

	Result GrafRenderer::Initialize(std::unique_ptr<GrafSystem>& grafSystem, const InitParams& initParams)
	{
		this->grafSystem.reset(grafSystem.release());
		if (ur_null == this->grafSystem.get())
			return ResultError(InvalidArgs, "GrafRenderer: failed to initialize, invalid GrafSystem");

		Result res(Success);
		const char* crntStageLogName = "";
		do
		{
			// graphics device
			crntStageLogName = "GrafDevice";
			res = this->grafSystem->CreateDevice(this->grafDevice);
			if (Failed(res)) break;
			ur_uint deviceId = (InitParams::RecommendedDeviceId == initParams.DeviceId ? this->grafSystem->GetRecommendedDeviceId() : initParams.DeviceId);
			res = this->grafDevice->Initialize(deviceId);
			if (Failed(res)) break;

			// presentation canvas (swapchain)
			crntStageLogName = "GrafCanvas";
			res = this->grafSystem->CreateCanvas(this->grafCanvas);
			if (Failed(res)) break;
			this->grafCanvasParams = initParams.CanvasParams;
			res = this->grafCanvas->Initialize(this->grafDevice.get(), initParams.CanvasParams);
			if (Failed(res)) break;

			// number of recorded (in flight) frames
			// can differ from swap chain size
			this->frameCount = std::max(ur_uint(2), this->grafCanvas->GetSwapChainImageCount() - 1);
			this->frameIdx = 0;

			// swap chain render pass
			// used to render into swap chain render target(s)
			crntStageLogName = "swap chain render pass";
			res = this->grafSystem->CreateRenderPass(this->grafCanvasRenderPass);
			if (Failed(res)) break;
			res = this->grafCanvasRenderPass->Initialize(this->grafDevice.get());
			if (Failed(res)) break;

			// swap chain image(s) render target(s)
			crntStageLogName = "swap chain render target(s)";
			res = this->InitializeCanvasRenderTargets();
			if (Failed(res)) break;

			// dynamic upload buffer
			crntStageLogName = "dynamic upload buffer";
			res = this->grafSystem->CreateBuffer(this->grafDynamicUploadBuffer);
			if (Failed(res)) break;
			GrafBufferDesc uploadBufferDesc;
			uploadBufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::TransferSrc;
			uploadBufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
			uploadBufferDesc.SizeInBytes = initParams.DynamicUploadBufferSize;
			res = this->grafDynamicUploadBuffer->Initialize(this->grafDevice.get(), { uploadBufferDesc });
			if (Failed(res)) break;
			this->uploadBufferAllocator.Init(uploadBufferDesc.SizeInBytes);

			// dynamic constant buffer
			crntStageLogName = "dynamic constant buffer";
			res = this->grafSystem->CreateBuffer(this->grafDynamicConstantBuffer);
			if (Failed(res)) break;
			GrafBufferDesc constantBufferDesc;
			constantBufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::ConstantBuffer;
			constantBufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
			constantBufferDesc.SizeInBytes = initParams.DynamicConstantBufferSize;
			res = this->grafDynamicConstantBuffer->Initialize(this->grafDevice.get(), { constantBufferDesc });
			if (Failed(res)) break;
			this->constantBufferAllocator.Init(constantBufferDesc.SizeInBytes, this->grafDevice->GetPhysicalDeviceDesc()->ConstantBufferOffsetAlignment);

		} while (false);
		
		if (Failed(res))
		{
			this->Deinitialize();
			LogError(std::string("GrafRenderer: failed to initialize ") + crntStageLogName);
		}
		
		return res;
	}

	Result GrafRenderer::InitializeCanvasRenderTargets()
	{
		Result res = NotInitialized;

		// RT count must be equal to swap chain size (one RT wraps on swap chain image)
		this->grafCanvasRenderTarget.clear();
		this->grafCanvasRenderTarget.resize(this->grafCanvas->GetSwapChainImageCount());
		for (ur_uint imageIdx = 0; imageIdx < this->grafCanvas->GetSwapChainImageCount(); ++imageIdx)
		{
			res = this->grafSystem->CreateRenderTarget(this->grafCanvasRenderTarget[imageIdx]);
			if (Failed(res)) break;
			GrafImage* renderTargetImages[] = {
				this->grafCanvas->GetSwapChainImage(imageIdx)
			};
			GrafRenderTarget::InitParams renderTargetParams = {};
			renderTargetParams.RenderPass = this->grafCanvasRenderPass.get();
			renderTargetParams.Images = renderTargetImages;
			renderTargetParams.ImageCount = ur_array_size(renderTargetImages);
			res = this->grafCanvasRenderTarget[imageIdx]->Initialize(this->grafDevice.get(), renderTargetParams);
			if (Failed(res)) break;
		}
		
		return res;
	}

	Result GrafRenderer::Deinitialize()
	{
		// order matters!

		// make sure there are no resources still used on gpu before destroying
		if (this->grafDevice != ur_null)
		{
			this->grafDevice->WaitIdle();
		}

		this->grafDynamicConstantBuffer.reset();
		this->grafDynamicUploadBuffer.reset();
		this->grafCanvasRenderTarget.clear();
		this->grafCanvasRenderPass.reset();
		this->grafCanvas.reset();
		this->grafDevice.reset();
		this->grafSystem.reset();

		return Result(Success);
	}

	Result GrafRenderer::BeginFrame()
	{
		if (ur_null == this->grafCanvas)
			return Result(NotInitialized);

		// update swapchain resolution
		ur_uint swapchainWidth = 0;
		ur_uint swapchainHeight = 0;
		if (this->grafCanvas->GetCurrentImage() != ur_null)
		{
			swapchainWidth = this->grafCanvas->GetCurrentImage()->GetDesc().Size.x;
			swapchainHeight = this->grafCanvas->GetCurrentImage()->GetDesc().Size.y;
		}
		if (swapchainWidth != this->GetRealm().GetCanvas()->GetClientBound().Width() ||
			swapchainHeight != this->GetRealm().GetCanvas()->GetClientBound().Height())
		{
			if (this->GetRealm().GetCanvas()->GetClientBound().Area() > 0)
			{
				Result res = this->grafCanvas->Initialize(this->grafDevice.get(), this->grafCanvasParams);
				if (Succeeded(res))
				{
					res = this->InitializeCanvasRenderTargets();
				}
				if (Failed(res))
				{
					return ResultError(res.Code, "GrafRenderer: failed to resize swap chain");
				}
			}
		}

		return Result(Success);
	}

	Result GrafRenderer::EndFrameAndPresent()
	{
		// process pending callbacks
		this->ProcessPendingCommandListCallbacks();

		// present current swap chain image
		Result res = this->grafCanvas->Present();

		// move to next frame
		this->frameIdx = (this->frameIdx + 1) % this->frameCount;

		return res;
	}

	Result GrafRenderer::AddCommandListCallback(GrafCommandList *executionCmdList, GrafCallbackContext ctx, GrafCommandListCallback callback)
	{
		if (ur_null == executionCmdList)
			return Result(InvalidArgs);

		if (executionCmdList->Wait(0) == Success)
		{
			// command list is not in pending state, call back right now
			callback(ctx);
		}
		else
		{
			// command list fence is not signaled, add to pending list
			std::unique_ptr<PendingCommandListCallbackData> pendingCallback(new PendingCommandListCallbackData());
			pendingCallback->cmdList = executionCmdList;
			pendingCallback->callback = callback;
			pendingCallback->context = ctx;
			this->pendingCommandListCallbacks.push_back(std::move(pendingCallback));
		}

		return Result(Success);
	}

	Result GrafRenderer::ProcessPendingCommandListCallbacks()
	{
		Result res(Success);

		ur_size idx = 0;
		while (idx < this->pendingCommandListCallbacks.size())
		{
			auto& pendingCallback = this->pendingCommandListCallbacks[idx];
			if (pendingCallback->cmdList->Wait(0) == Success)
			{
				// command list is finished
				res &= pendingCallback->callback(pendingCallback->context);
				this->pendingCommandListCallbacks.erase(this->pendingCommandListCallbacks.begin() + idx);
			}
			else
			{
				// still pending
				++idx;
			}
		}

		return res;
	}

	Result GrafRenderer::Upload(ur_byte *dataPtr, ur_size dataSize, GrafImage* dstImage, GrafImageState dstImageState)
	{
		if (ur_null == dataPtr || 0 == dataSize || ur_null == dstImage)
			return Result(InvalidArgs);

		// allocate
		Allocation uploadBufferAlloc = this->uploadBufferAllocator.Allocate(dataSize);
		if (0 == uploadBufferAlloc.Size)
			return Result(OutOfMemory);

		// write data to cpu visible dynamic upload buffer
		Result res = this->grafDynamicUploadBuffer->Write(dataPtr, dataSize, 0, uploadBufferAlloc.Offset);
		if (Failed(res))
			return ResultError(Failure, "GrafRenderer: failed to write to upload buffer");

		return this->Upload(this->grafDynamicUploadBuffer.get(), uploadBufferAlloc.Offset, dstImage, dstImageState);
	}

	Result GrafRenderer::Upload(GrafBuffer *srcBuffer, ur_size srcOffset, GrafImage* dstImage, GrafImageState dstImageState)
	{
		// create upload command list
		std::unique_ptr<GrafCommandList> grafUploadCmdList;
		Result res = this->grafSystem->CreateCommandList(grafUploadCmdList);
		if (Succeeded(res))
		{
			res = grafUploadCmdList->Initialize(this->grafDevice.get());
		}
		if (Failed(res))
			return ResultError(Failure, "ImguiRender::Init: failed to create upload command list");

		// copy dynamic buffer region to destination image
		grafUploadCmdList->Begin();
		grafUploadCmdList->ImageMemoryBarrier(dstImage, GrafImageState::Current, GrafImageState::TransferDst);
		grafUploadCmdList->Copy(srcBuffer, dstImage, srcOffset);
		grafUploadCmdList->ImageMemoryBarrier(dstImage, GrafImageState::Current, dstImageState);
		grafUploadCmdList->End();
		this->grafDevice->Submit(grafUploadCmdList.get());

		// destroy upload command list when finished
		GrafCommandList* uploadCmdListPtr = grafUploadCmdList.release();
		this->AddCommandListCallback(uploadCmdListPtr, { uploadCmdListPtr }, [](GrafCallbackContext& ctx) -> Result
		{
			GrafCommandList* finishedUploadCmdList = reinterpret_cast<GrafCommandList*>(ctx.DataPtr);
			delete finishedUploadCmdList;
			return Result(Success);
		});

		return Result(Success);
	}

	Result GrafRenderer::ShowImgui()
	{
		if (ur_null == this->grafSystem)
			return Result(NotInitialized);

		const RectI& canvasRect = this->GetRealm().GetCanvas()->GetClientBound();
		const ImVec2 windowSize(400.0f, (float)canvasRect.Height());
		ImGui::SetNextWindowSize(windowSize, ImGuiSetCond_Once);
		ImGui::SetNextWindowPos({ canvasRect.Width() - windowSize.x, 0.0f }, ImGuiSetCond_Once);
		ImGui::Begin("GrafRenderer");
		ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
		if (ImGui::TreeNode("GrafSystem"))
		{
			ImGui::Text("Implementation: %s", typeid(*this->grafSystem.get()).name());
			ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
			if (ImGui::TreeNode("Devices Available:"))
			{
				for (ur_uint idevice = 0; idevice < this->grafSystem->GetPhysicalDeviceCount(); ++idevice)
				{
					const GrafPhysicalDeviceDesc* deviceDesc = this->grafSystem->GetPhysicalDeviceDesc(idevice);
					ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
					if (ImGui::TreeNode("PhysicalDeviceNode", "%s", deviceDesc->Description.c_str()))
					{
						ImGui::Text("Dedicated Memory (Mb): %u", deviceDesc->DedicatedVideoMemory / (1 << 20));
						ImGui::Text("Shared Memory (Mb): %u", deviceDesc->SharedSystemMemory / (1 << 20));
						ImGui::TreePop();
					}
				}
				ImGui::TreePop();
			}
			if (this->grafDevice != ur_null)
			{
				ImGui::Text("Device used: %s", this->grafDevice->GetPhysicalDeviceDesc()->Description.c_str());
			}
			if (this->grafCanvas != ur_null)
			{
				ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
				if (ImGui::TreeNode("Swap Chain:"))
				{
					ImGui::Text("Image count: %i", this->grafCanvas->GetSwapChainImageCount());
					if (this->grafCanvas->GetSwapChainImageCount() > 0)
					{
						const GrafImageDesc& imageDesc = this->grafCanvas->GetCurrentImage()->GetDesc();
						ImGui::Text("Image Size: %i x %i", imageDesc.Size.x, imageDesc.Size.y);
					}
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
		if (this->grafDynamicUploadBuffer)
		{
			static ur_size prevAllocatorOffset = 0;
			static ur_size allocatorOffsetDeltaMax = 0;
			ur_size crntAllocatorOffset = this->uploadBufferAllocator.GetOffset();
			ur_size allocatorOffsetDelta = crntAllocatorOffset + (crntAllocatorOffset < prevAllocatorOffset ? this->uploadBufferAllocator.GetSize() : 0) - prevAllocatorOffset;
			allocatorOffsetDeltaMax = std::max(allocatorOffsetDelta, allocatorOffsetDeltaMax);
			prevAllocatorOffset = crntAllocatorOffset;
			ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
			if (ImGui::TreeNode("Dynamic Upload Buffer"))
			{
				ImGui::Text("Size: %i", this->grafDynamicUploadBuffer->GetDesc().SizeInBytes);
				ImGui::Text("Maximal usage per frame: %i", allocatorOffsetDeltaMax);
				ImGui::Text("Current frame usage: %i", allocatorOffsetDelta);
				ImGui::Text("Current ofs: %i", this->uploadBufferAllocator.GetOffset());
				ImGui::TreePop();
			}
		}
		if (this->grafDynamicConstantBuffer)
		{
			static ur_size prevAllocatorOffset = 0;
			static ur_size allocatorOffsetDeltaMax = 0;
			ur_size crntAllocatorOffset = this->constantBufferAllocator.GetOffset();
			ur_size allocatorOffsetDelta = crntAllocatorOffset + (crntAllocatorOffset < prevAllocatorOffset ? this->constantBufferAllocator.GetSize() : 0) - prevAllocatorOffset;
			allocatorOffsetDeltaMax = std::max(allocatorOffsetDelta, allocatorOffsetDeltaMax);
			prevAllocatorOffset = crntAllocatorOffset;
			ImGui::SetNextTreeNodeOpen(true, ImGuiSetCond_Once);
			if (ImGui::TreeNode("Dynamic Constant Buffer"))
			{
				ImGui::Text("Size: %i", this->grafDynamicConstantBuffer->GetDesc().SizeInBytes);
				ImGui::Text("Maximal usage per frame: %i", allocatorOffsetDeltaMax);
				ImGui::Text("Current frame usage: %i", allocatorOffsetDelta);
				ImGui::Text("Current ofs (bytes): %i", this->constantBufferAllocator.GetOffset());
				ImGui::TreePop();
			}
		}
		ImGui::Text("Recorded frame count: %i", this->frameCount);
		ImGui::End();

		return Result(Success);
	}

} // end namespace UnlimRealms