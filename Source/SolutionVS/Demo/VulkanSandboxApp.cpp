
#include "stdafx.h"
#include "VulkanSandboxApp.h"

#include "UnlimRealms.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"
#include "Sys/JobSystem.h"
#include "Sys/Windows/WinCanvas.h"
#include "Sys/Windows/WinInput.h"
#include "Resources/Resources.h"
#include "Core/Math.h"
#include "ImguiRender/ImguiRender.h"
#pragma comment(lib, "UnlimRealms.lib")
using namespace UnlimRealms;

int VulkanSandboxApp::Run()
{
	// create realm
	Realm realm;
	realm.Initialize();

	// create system canvas
	std::unique_ptr<WinCanvas> canvas(new WinCanvas(realm, WinCanvas::Style::OverlappedWindowMaximized, L"Vulkan Sandbox"));
	canvas->Initialize(RectI(0, 0, (ur_uint)GetSystemMetrics(SM_CXSCREEN), (ur_uint)GetSystemMetrics(SM_CYSCREEN)));
	realm.SetCanvas(std::move(canvas));
	ur_uint canvasWidth = realm.GetCanvas()->GetClientBound().Width();
	ur_uint canvasHeight = realm.GetCanvas()->GetClientBound().Height();
	ur_bool canvasValid = (realm.GetCanvas()->GetClientBound().Area() > 0);

	// create input system
	std::unique_ptr<WinInput> input(new WinInput(realm));
	input->Initialize();
	realm.SetInput(std::move(input));

	// load shader data
	std::unique_ptr<ur_byte[]> sampleVSBuffer;
	ur_size sampleVSBufferSize = 0;
	{
		std::unique_ptr<File> file;
		Result res = realm.GetStorage().Open(file, "sample_vs.spv", ur_uint(StorageAccess::Read) | ur_uint(StorageAccess::Binary));
		if (Succeeded(res))
		{
			sampleVSBufferSize = file->GetSize();
			sampleVSBuffer.reset(new ur_byte[sampleVSBufferSize]);
			file->Read(sampleVSBufferSize, sampleVSBuffer.get());
		}
	}
	std::unique_ptr<ur_byte[]> samplePSBuffer;
	ur_size samplePSBufferSize = 0;
	{
		std::unique_ptr<File> file;
		Result res = realm.GetStorage().Open(file, "sample_ps.spv", ur_uint(StorageAccess::Read) | ur_uint(StorageAccess::Binary));
		if (Succeeded(res))
		{
			samplePSBufferSize = file->GetSize();
			samplePSBuffer.reset(new ur_byte[samplePSBufferSize]);
			file->Read(samplePSBufferSize, samplePSBuffer.get());
		}
	}

	// sample mesh data
	struct VertexSample
	{
		ur_float3 pos;
		ur_float3 color;
	};
	VertexSample verticesSample[] = {
		{ { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
		{ { 0.5f,-0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
		{ {-0.5f,-0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
	};

	// initialize gfx system
	std::unique_ptr<GrafSystem> grafSystem(new GrafSystemVulkan(realm));
	std::unique_ptr<GrafDevice> grafDevice;
	std::unique_ptr<GrafCanvas> grafCanvas;
	std::unique_ptr<GrafShader> grafShaderSampleVS;
	std::unique_ptr<GrafShader> grafShaderSamplePS;
	std::unique_ptr<GrafBuffer> grafVBSample;
	std::unique_ptr<GrafRenderPass> grafRenderPassSample;
	std::unique_ptr<GrafPipeline> grafPipelineSample;
	std::vector<std::unique_ptr<GrafRenderTarget>> grafRenderTarget;
	std::vector<std::unique_ptr<GrafCommandList>> grafMainCmdList;
	std::vector<std::unique_ptr<GrafBuffer>> grafCBSample;
	ur_uint frameCount = 0;
	ur_uint frameIdx = 0;
	auto& deinitializeGfxFrameObjects = [&]() -> void {
		// order matters!
		if (grafDevice.get()) grafDevice->WaitIdle();
		grafRenderTarget.clear();
		grafMainCmdList.clear();
		grafCBSample.clear();
	};
	auto& deinitializeGfxSystem = [&]() -> void {
		// order matters!
		deinitializeGfxFrameObjects();
		grafPipelineSample.reset();
		grafRenderPassSample.reset();
		grafVBSample.reset();
		grafShaderSampleVS.reset();
		grafShaderSamplePS.reset();
		grafCanvas.reset();
		grafDevice.reset();
		grafSystem.reset();
	};
	Result grafRes = NotInitialized;
	auto& initializeGfxSystem = [&]() -> void
	{
		grafRes = grafSystem->Initialize(realm.GetCanvas());
		if (Failed(grafRes) || 0 == grafSystem->GetPhysicalDeviceCount()) return;
		
		grafRes = grafSystem->CreateDevice(grafDevice);
		if (Failed(grafRes)) return;
		grafRes = grafDevice->Initialize(grafSystem->GetRecommendedDeviceId());
		if (Failed(grafRes)) return;

		grafRes = grafSystem->CreateCanvas(grafCanvas);
		if (Failed(grafRes)) return;
		grafRes = grafCanvas->Initialize(grafDevice.get());
		if (Failed(grafRes)) return;

		grafRes = grafSystem->CreateShader(grafShaderSampleVS);
		if (Failed(grafRes)) return;
		grafRes = grafShaderSampleVS->Initialize(grafDevice.get(), { GrafShaderType::Vertex, sampleVSBuffer.get(), sampleVSBufferSize, GrafShader::DefaultEntryPoint });
		if (Failed(grafRes)) return;

		grafRes = grafSystem->CreateShader(grafShaderSamplePS);
		if (Failed(grafRes)) return;
		grafRes = grafShaderSamplePS->Initialize(grafDevice.get(), { GrafShaderType::Pixel, samplePSBuffer.get(), samplePSBufferSize, GrafShader::DefaultEntryPoint });
		if (Failed(grafRes)) return;

		grafRes = grafSystem->CreateBuffer(grafVBSample);
		if (Failed(grafRes)) return;
		GrafBuffer::InitParams grafVBSampleParams;
		grafVBSampleParams.BufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::VertexBuffer;
		grafVBSampleParams.BufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
		grafVBSampleParams.BufferDesc.SizeInBytes = sizeof(verticesSample);
		grafRes = grafVBSample->Initialize(grafDevice.get(), grafVBSampleParams);
		if (Failed(grafRes)) return;
		grafRes = grafVBSample->Write((ur_byte*)verticesSample);
		if (Failed(grafRes)) return;

		grafRes = grafSystem->CreateRenderPass(grafRenderPassSample);
		if (Failed(grafRes)) return;
		grafRes = grafRenderPassSample->Initialize(grafDevice.get());
		if (Failed(grafRes)) return;

		grafRes = grafSystem->CreatePipeline(grafPipelineSample);
		if (Failed(grafRes)) return;
		GrafShader* sampleShaderStages[] = {
			grafShaderSampleVS.get(),
			grafShaderSamplePS.get()
		};
		GrafVertexElementDesc sampleVertexElements[] = {
			{ GrafFormat::R32G32B32_SFLOAT, 0 },
			{ GrafFormat::R32G32B32_SFLOAT, 12 }
		};
		GrafVertexInputDesc sampleVertexInputs[] = {{
			GrafVertexInputType::PerVertex, 0, sizeof(VertexSample),
			ur_array_size(sampleVertexElements), sampleVertexElements
		}};
		GrafPipeline::InitParams samplePipelineParams = GrafPipeline::InitParams::Default;
		samplePipelineParams.RenderPass = grafRenderPassSample.get();
		samplePipelineParams.ShaderStageCount = ur_array_size(sampleShaderStages);
		samplePipelineParams.ShaderStages = sampleShaderStages;
		samplePipelineParams.VertexInputCount = ur_array_size(sampleVertexInputs);
		samplePipelineParams.VertexInputDesc = sampleVertexInputs;
		grafRes = grafPipelineSample->Initialize(grafDevice.get(), samplePipelineParams);
		if (Failed(grafRes)) return;
	};
	auto& initializeGfxFrameObjects = [&]() -> void
	{
		if (Failed(grafRes)) return;

		// RT count must be equal to swap chain size (one RT wraps on swap chain image)
		grafRenderTarget.resize(grafCanvas->GetSwapChainImageCount());
		for (ur_uint imageIdx = 0; imageIdx < grafCanvas->GetSwapChainImageCount(); ++imageIdx)
		{
			grafRes = grafSystem->CreateRenderTarget(grafRenderTarget[imageIdx]);
			if (Failed(grafRes)) break;
			GrafRenderTarget::InitParams renderTargetParams = {};
			renderTargetParams.RenderPass = grafRenderPassSample.get();
			renderTargetParams.Images.push_back(grafCanvas->GetSwapChainImage(imageIdx));
			grafRes = grafRenderTarget[imageIdx]->Initialize(grafDevice.get(), renderTargetParams);
			if (Failed(grafRes)) break;
		}
		if (Failed(grafRes)) return;

		// number of recorded frames can differ from swap chain size
		frameCount = std::max(ur_uint(1), grafCanvas->GetSwapChainImageCount() - 1);
		frameIdx = 0;
		grafMainCmdList.resize(frameCount);
		for (ur_uint iframe = 0; iframe < frameCount; ++iframe)
		{
			grafRes = grafSystem->CreateCommandList(grafMainCmdList[iframe]);
			if (Failed(grafRes)) break;
			grafRes = grafMainCmdList[iframe]->Initialize(grafDevice.get());
			if (Failed(grafRes)) break;
		}
		if (Failed(grafRes)) return;
				
		GrafBuffer::InitParams grafCBSampleParams;
		grafCBSampleParams.BufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::ConstantBuffer;
		grafCBSampleParams.BufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
		grafCBSampleParams.BufferDesc.SizeInBytes = sizeof(ur_float4x4);
		grafCBSample.resize(frameCount);
		for (ur_uint iframe = 0; iframe < frameCount; ++iframe)
		{
			grafRes = grafSystem->CreateBuffer(grafCBSample[iframe]);
			if (Failed(grafRes)) break;
			grafRes = grafCBSample[iframe]->Initialize(grafDevice.get(), grafCBSampleParams);
			if (Failed(grafRes)) break;
		}
		if (Failed(grafRes)) return;
	};
	initializeGfxSystem();
	if (Succeeded(grafRes))
	{
		initializeGfxFrameObjects();
	}
	if (Failed(grafRes))
	{
		deinitializeGfxSystem();
		realm.GetLog().WriteLine("VulkanSandboxApp: failed to initialize graphics system", Log::Error);
	}
	else
	{
		// TODO: GRAF system should be accessible from everywhere as a realm component;
		// requires keeping pointers to GrafDevice(s) and GrafCanvas(es) in GrafSystem...
		//realm.AddComponent(Component::GetUID<GrafSystem>(), std::unique_ptr<Component>(static_cast<Component*>(std::move(grafSystem))));
		//GrafSystem* grafSystem = realm.GetComponent<GrafSystem>();
	}

	// initialize sample primitive renering

	// initialize ImguiRender
	ImguiRender* imguiRender = realm.AddComponent<ImguiRender>(realm);
	if (imguiRender != ur_null)
	{
		imguiRender = realm.GetComponent<ImguiRender>();
		Result res = imguiRender->Init();
		if (Failed(res))
		{
			realm.RemoveComponent<ImguiRender>();
			imguiRender = ur_null;
			realm.GetLog().WriteLine("VulkanSandbox: failed to initialize ImguiRender", Log::Error);
		}
	}

	// animated clear color test
	static const ur_float clearColorDelay = 2.0f;
	static const ur_float4 clearColors[] = {
		{ 0.0f, 0.0f, 0.0f, 1.0f },
		{ 0.0f, 0.0f, 1.0f, 1.0f },
		{ 0.0f, 1.0f, 0.0f, 1.0f },
		{ 0.0f, 1.0f, 1.0f, 1.0f },
		{ 1.0f, 0.0f, 0.0f, 1.0f },
		{ 1.0f, 0.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 0.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f, 1.0f },
	};
	ur_uint crntColorId = 0;
	ur_uint nextColorId = (crntColorId + 1) % ur_array_size(clearColors);
	ur_float crntColorWeight = 0.0f;
	ur_float4 crntClearColor(0.0f);

	// sample animation
	static const ur_float sampleAnimDelay = 1.0f;
	static const ur_float sampleAnimRadius = 0.1f;
	ur_float sampleAnimElapsedTime = 0;
	ur_float2 sampleAnimPos(0.0f);

	// Main message loop:
	ClockTime timer = Clock::now();
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT && !realm.GetInput()->GetKeyboard()->IsKeyReleased(Input::VKey::Escape))
	{
		// process messages first
		ClockTime timeBefore = Clock::now();
		while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) && msg.message != WM_QUIT)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			// forward msg to WinInput system
			WinInput* winInput = static_cast<WinInput*>(realm.GetInput());
			winInput->ProcessMsg(msg);
		}
		if (msg.message == WM_QUIT)
			break;
		auto inputDelay = Clock::now() - timeBefore;
		timer += inputDelay;  // skip input delay

		// update resolution
		if (canvasWidth != realm.GetCanvas()->GetClientBound().Width() || canvasHeight != realm.GetCanvas()->GetClientBound().Height())
		{
			canvasWidth = realm.GetCanvas()->GetClientBound().Width();
			canvasHeight = realm.GetCanvas()->GetClientBound().Height();
			canvasValid = (realm.GetCanvas()->GetClientBound().Area() > 0);
			if (grafSystem != ur_null && canvasValid)
			{
				grafCanvas->Initialize(grafDevice.get());
				deinitializeGfxFrameObjects();
				initializeGfxFrameObjects();
			}
		}

		// update sub systems
		realm.GetInput()->Update();
		if (imguiRender != ur_null)
		{
			imguiRender->NewFrame();
		}

		// update frame
		auto updateFrameJob = realm.GetJobSystem().Add(ur_null, [&](Job::Context& ctx) -> void {
			
			// reset update progress fence
			ctx.progress = 0;

			// move timer
			ClockTime timeNow = Clock::now();
			auto deltaTime = ClockDeltaAs<std::chrono::microseconds>(timeNow - timer);
			timer = timeNow;
			ur_float elapsedTime = (float)deltaTime.count() * 1.0e-6f;  // to seconds

			// TODO: animate things here

			// clear color animation
			crntColorWeight += elapsedTime / clearColorDelay;
			if (crntColorWeight >= 1.0f)
			{
				ur_float intPart;
				crntColorWeight = std::modf(crntColorWeight, &intPart);
				crntColorId = (crntColorId + ur_uint(intPart)) % ur_array_size(clearColors);
				nextColorId = (crntColorId + 1) % ur_array_size(clearColors);
			}
			crntClearColor = ur_float4::Lerp(clearColors[crntColorId], clearColors[nextColorId], crntColorWeight);
			ctx.progress = 1;

			// sample aniamtion

			sampleAnimElapsedTime += elapsedTime;
			sampleAnimPos.x = cosf(sampleAnimElapsedTime / sampleAnimDelay * MathConst<ur_float>::Pi * 2.0f) * sampleAnimRadius;
			sampleAnimPos.y = sinf(sampleAnimElapsedTime / sampleAnimDelay * MathConst<ur_float>::Pi * 2.0f) * sampleAnimRadius;
			ctx.progress = 2;
		});

		// draw frame
		auto drawFrameJob = realm.GetJobSystem().Add(ur_null, [&](Job::Context& ctx) -> void {
			
			//updateFrameJob->WaitProgress(0);  // wait till portion of data required for this draw call is fully updated

			if (grafSystem != ur_null && canvasValid)
			{
				GrafCommandList* grafCmdListCrnt = grafMainCmdList[frameIdx].get();

				grafCmdListCrnt->Begin();
			
				updateFrameJob->WaitProgress(1); // make sure required data is ready
				grafCmdListCrnt->ClearColorImage(grafCanvas->GetCurrentImage(), reinterpret_cast<GrafClearValue&>(crntClearColor));
				
				GrafViewportDesc grafViewport = {};
				grafViewport.Width = (ur_float)grafCanvas->GetCurrentImage()->GetDesc().Size.x;
				grafViewport.Height = (ur_float)grafCanvas->GetCurrentImage()->GetDesc().Size.y;
				grafViewport.Near = 0.0f;
				grafViewport.Far = 1.0f;
				grafCmdListCrnt->SetViewport(grafViewport, true);

				// TEMP: constant buffers test
				updateFrameJob->WaitProgress(2); // wait till animation params are up to date
				GrafPipelineVulkan* grafPipelineVulkan = static_cast<GrafPipelineVulkan*>(grafPipelineSample.get());
				ur_float4x4 testTransform = ur_float4x4::Identity;
				testTransform.r[3].x = sampleAnimPos.x;
				testTransform.r[3].y = sampleAnimPos.y;
				grafCBSample[frameIdx]->Write((ur_byte*)&testTransform);
				grafPipelineVulkan->UpdateConstantBuffer(frameIdx, grafCBSample[frameIdx].get());
				grafPipelineVulkan->BindDescriptorSet(frameIdx, grafCmdListCrnt);

				grafCmdListCrnt->BeginRenderPass(grafRenderPassSample.get(), grafRenderTarget[grafCanvas->GetCurrentImageId()].get());
				grafCmdListCrnt->BindPipeline(grafPipelineSample.get());
				grafCmdListCrnt->BindVertexBuffer(grafVBSample.get(), 0);
				grafCmdListCrnt->Draw(3, 4, 0, 0);
				grafCmdListCrnt->EndRenderPass();

				grafCmdListCrnt->End();

				grafDevice->Submit(grafCmdListCrnt);
			}
		});

		drawFrameJob->Wait();

		if (grafSystem != ur_null && canvasValid)
		{
			// present & move to next frame
			grafCanvas->Present();

			// start recording next frame
			frameIdx = (frameIdx + 1) % frameCount;
		}
	}

	deinitializeGfxSystem();

	return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GRAF: GRaphics Abstraction Front-end
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

Result GrafCommandList::BindVertexBuffer(GrafBuffer* grafVertexBuffer, ur_uint bindingIdx)
{
	return Result(NotImplemented);
}

Result GrafCommandList::Draw(ur_uint vertexCount, ur_uint instanceCount, ur_uint firstVertex, ur_uint firstInstance)
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

Result GrafBuffer::Read(ur_byte*& dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
{
	return Result(NotImplemented);
}

Result GrafBuffer::Transfer(GrafBuffer* dstBuffer, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
{
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
	this->images.resize(initParams.Images.size());
	memcpy(this->images.data(), initParams.Images.data(), initParams.Images.size() * sizeof(GrafImage*));
	return Result(NotImplemented);
}

const GrafPipeline::InitParams GrafPipeline::InitParams::Default = {
	ur_null, // RenderPass
	0, // ShaderStageCount
	ur_null, // GrafShader**
	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f }, //GrafViewportDesc
	GrafPrimitiveTopology::TriangleList, // GrafPrimitiveTopology
	0, // VertexInputCount
	ur_null, // GrafVertexInputDesc
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

Result SetViewport(const GrafViewportDesc& viewportDesc)
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

Result GrafRenderPass::Initialize(GrafDevice* grafDevice)
{
	GrafDeviceEntity::Initialize(grafDevice);
	return Result(NotImplemented);
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GRAF: VULKAN IMPLEMENTATION
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma comment(lib, "vulkan-1.lib")

#if defined(_DEBUG)
#define UR_GRAF_LOG_LEVEL_DEBUG
#define UR_GRAF_VULKAN_DEBUG_LAYER
#endif

#define UR_GRAF_VULKAN_IMPLICIT_WAIT_DEVICE 1

// vulkan objects destruction safety policy
#define UR_GRAF_VULKAN_DESTROY_DO_NOT_WAIT 0		// destroy immediately, do not check synchronization objects
#define UR_GRAF_VULKAN_DESTROY_WAIT_IMMEDIATE 1		// stall thread and wait synchronization object
#define UR_GRAF_VULKAN_DESTROY_WAIT_DEFERRED 0		// TODO: check if vulkan object is still used on device, and place it into deferred deinitialization list if it is

// enables swap chain image layout automatic transition to general/common state when it becomes a current render target
#define UR_GRAF_VULKAN_SWAPCHAIN_NEXT_IMAGE_IMPLICIT_TRANSITION_TO_GENERAL 1

#if defined(UR_GRAF_LOG_LEVEL_DEBUG)
#define LogNoteGrafDbg(text) GetRealm().GetLog().WriteLine(text, Log::Note)
#else
#define LogNoteGrafDbg(text)
#endif

#if defined(UR_GRAF_VULKAN_DEBUG_LAYER)
static const char* VulkanLayers[] = {
	"VK_LAYER_KHRONOS_validation"
};
#else
static const char** VulkanLayers = ur_null;
#endif

static const char* VulkanExtensions[] = {
	"VK_KHR_surface"
#if defined(_WINDOWS)
	, "VK_KHR_win32_surface"
#endif
#if defined(UR_GRAF_VULKAN_DEBUG_LAYER)
	, "VK_EXT_debug_utils"
#endif
};

static const char* VulkanDeviceExtensions[] = {
	"VK_KHR_swapchain"
};

static const char* VkResultToString(VkResult res)
{
	switch (res)
	{
	case VK_SUCCESS: return "VK_SUCCESS";
	case VK_NOT_READY: return "VK_NOT_READY";
	case VK_TIMEOUT: return "VK_TIMEOUT";
	case VK_EVENT_SET: return "VK_EVENT_SET";
	case VK_EVENT_RESET: return "VK_EVENT_RESET";
	case VK_INCOMPLETE: return "VK_INCOMPLETE";
	case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
	case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
	case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
	case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
	case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
	case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
	case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
	case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
	case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
	case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
	case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
	case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
	case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
	case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
	case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
	case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
	case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
	case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
	case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
	case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
	case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
	case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
	case VK_ERROR_FRAGMENTATION_EXT: return "VK_ERROR_FRAGMENTATION_EXT";
	case VK_ERROR_NOT_PERMITTED_EXT: return "VK_ERROR_NOT_PERMITTED_EXT";
	case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT: return "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT";
	case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
	};
	return "VK_UNKNOWN";
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GrafSystemVulkan::GrafSystemVulkan(Realm &realm) : 
	GrafSystem(realm)
{
	this->vkInstance = VK_NULL_HANDLE;
}

GrafSystemVulkan::~GrafSystemVulkan()
{
	this->Deinitialize();
}

Result GrafSystemVulkan::Deinitialize()
{
	this->grafPhysicalDeviceDesc.clear();
	this->vkPhysicalDevices.clear();

	if (this->vkInstance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(this->vkInstance, ur_null);
		this->vkInstance = VK_NULL_HANDLE;
		LogNoteGrafDbg("GrafSystemVulkan: deinitialized");
	}

	return Result(Success);
}

Result GrafSystemVulkan::Initialize(Canvas *canvas)
{
	this->Deinitialize();

	LogNoteGrafDbg("GrafSystemVulkan: initialization...");
	
	#if defined(UR_GRAF_LOG_LEVEL_DEBUG)
	
	// enumerate extentions

	ur_uint32 vkExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(ur_null, &vkExtensionCount, ur_null);
	std::vector<VkExtensionProperties> vkExtensionProperties(vkExtensionCount);
	vkEnumerateInstanceExtensionProperties(ur_null, &vkExtensionCount, vkExtensionProperties.data());
	LogNoteGrafDbg("GrafSystemVulkan: extensions available:");
	for (auto& extensionProperty : vkExtensionProperties)
	{
		LogNoteGrafDbg(extensionProperty.extensionName);
	}

	// enumerate layers

	ur_uint32 vkLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&vkLayerCount, ur_null);
	std::vector<VkLayerProperties> vkLayerProperties(vkLayerCount);
	vkEnumerateInstanceLayerProperties(&vkLayerCount, vkLayerProperties.data());
	LogNoteGrafDbg("GrafSystemVulkan: layers available:");
	for (auto& layerProperty : vkLayerProperties)
	{
		LogNoteGrafDbg(std::string(layerProperty.layerName) + " (" + layerProperty.description + ")");
	}

	// NOTE: consider setting up custom debug callback for vulkan messages
	
	#endif

	// create instance

	VkApplicationInfo vkAppInfo = {};
	vkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	vkAppInfo.pApplicationName = "UnlimRealms Application";
	vkAppInfo.pEngineName = "UnlimRealms";
	vkAppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	vkAppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	vkAppInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo vkCreateInfo = {};
	vkCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vkCreateInfo.flags = 0;
	vkCreateInfo.pApplicationInfo = &vkAppInfo;
	vkCreateInfo.enabledLayerCount = (VulkanLayers ? ur_array_size(VulkanLayers) : 0);
	vkCreateInfo.ppEnabledLayerNames = VulkanLayers;
	vkCreateInfo.enabledExtensionCount = (VulkanExtensions ? ur_array_size(VulkanExtensions) : 0);
	vkCreateInfo.ppEnabledExtensionNames = VulkanExtensions;

	VkResult res = vkCreateInstance(&vkCreateInfo, ur_null, &this->vkInstance);
	if (res != VK_SUCCESS)
	{
		return ResultError(Failure, std::string("GrafSystemVulkan: vkCreateInstance failed with VkResult = ") + VkResultToString(res));
	}
	LogNoteGrafDbg("GrafSystemVulkan: VkInstance created");

	// enumerate physical devices

	ur_uint32 physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(this->vkInstance, &physicalDeviceCount, ur_null);
	if (0 == physicalDeviceCount)
	{
		this->Deinitialize();
		return ResultError(Failure, "GrafSystemVulkan: init failed, no physical device found");
	}
	this->vkPhysicalDevices.resize(physicalDeviceCount);
	vkEnumeratePhysicalDevices(this->vkInstance, &physicalDeviceCount, vkPhysicalDevices.data());
	this->grafPhysicalDeviceDesc.resize(physicalDeviceCount);
	for (ur_uint32 deviceId = 0; deviceId < physicalDeviceCount; ++deviceId)
	{
		VkPhysicalDevice& vkPhysicalDevice = vkPhysicalDevices[deviceId];
		VkPhysicalDeviceProperties vkDeviceProperties;
		VkPhysicalDeviceMemoryProperties vkDeviceMemoryProperties;
		vkGetPhysicalDeviceProperties(vkPhysicalDevice, &vkDeviceProperties);
		vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkDeviceMemoryProperties);
		
		GrafPhysicalDeviceDesc& grafDeviceDesc = this->grafPhysicalDeviceDesc[deviceId];
		grafDeviceDesc = {};
		grafDeviceDesc.Description = vkDeviceProperties.deviceName;
		grafDeviceDesc.VendorId = (ur_uint)vkDeviceProperties.vendorID;
		grafDeviceDesc.DeviceId = (ur_uint)vkDeviceProperties.deviceID;
		std::vector<VkMemoryPropertyFlags> perHeapFlags(vkDeviceMemoryProperties.memoryHeapCount);
		for (ur_uint32 memTypeIdx = 0; memTypeIdx < vkDeviceMemoryProperties.memoryTypeCount; ++memTypeIdx)
		{
			VkMemoryType& vkMemType = vkDeviceMemoryProperties.memoryTypes[memTypeIdx];
			perHeapFlags[vkMemType.heapIndex] |= vkMemType.propertyFlags;
		}
		for (ur_uint32 memHeapIdx = 0; memHeapIdx < vkDeviceMemoryProperties.memoryHeapCount; ++memHeapIdx)
		{
			VkMemoryHeap& vkMemHeap = vkDeviceMemoryProperties.memoryHeaps[memHeapIdx];
			if (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT & perHeapFlags[memHeapIdx]) grafDeviceDesc.DedicatedVideoMemory += (ur_size)vkMemHeap.size;
			if (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & perHeapFlags[memHeapIdx]) grafDeviceDesc.SharedSystemMemory += (ur_size)vkMemHeap.size;
		}
		
		#if defined(UR_GRAF_LOG_LEVEL_DEBUG)
		LogNoteGrafDbg(std::string("GrafSystemVulkan: device available ") + grafDeviceDesc.Description +
			", VRAM = " + std::to_string(grafDeviceDesc.DedicatedVideoMemory >> 20) + " Mb");
		#endif
	}

	return Result(Success);
}

Result GrafSystemVulkan::CreateDevice(std::unique_ptr<GrafDevice>& grafDevice)
{
	grafDevice.reset(new GrafDeviceVulkan(*this));
	return Result(Success);
}

Result GrafSystemVulkan::CreateCommandList(std::unique_ptr<GrafCommandList>& grafCommandList)
{
	grafCommandList.reset(new GrafCommandListVulkan(*this));
	return Result(Success);
}

Result GrafSystemVulkan::CreateFence(std::unique_ptr<GrafFence>& grafFence)
{
	grafFence.reset(new GrafFenceVulkan(*this));
	return Result(Success);
}

Result GrafSystemVulkan::CreateCanvas(std::unique_ptr<GrafCanvas>& grafCanvas)
{
	grafCanvas.reset(new GrafCanvasVulkan(*this));
	return Result(Success);
}

Result GrafSystemVulkan::CreateImage(std::unique_ptr<GrafImage>& grafImage)
{
	grafImage.reset(new GrafImageVulkan(*this));
	return Result(Success);
}

Result GrafSystemVulkan::CreateBuffer(std::unique_ptr<GrafBuffer>& grafBuffer)
{
	grafBuffer.reset(new GrafBufferVulkan(*this));
	return Result(Success);
}

Result GrafSystemVulkan::CreateShader(std::unique_ptr<GrafShader>& grafShader)
{
	grafShader.reset(new GrafShaderVulkan(*this));
	return Result(Success);
}

Result GrafSystemVulkan::CreateRenderPass(std::unique_ptr<GrafRenderPass>& grafRenderPass)
{
	grafRenderPass.reset(new GrafRenderPassVulkan(*this));
	return Result(Success);
}

Result GrafSystemVulkan::CreateRenderTarget(std::unique_ptr<GrafRenderTarget>& grafRenderTarget)
{
	grafRenderTarget.reset(new GrafRenderTargetVulkan(*this));
	return Result(Success);
}

Result GrafSystemVulkan::CreatePipeline(std::unique_ptr<GrafPipeline>& grafPipeline)
{
	grafPipeline.reset(new GrafPipelineVulkan(*this));
	return Result(Success);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GrafDeviceVulkan::GrafDeviceVulkan(GrafSystem &grafSystem) :
	GrafDevice(grafSystem)
{
	this->vkDevice = VK_NULL_HANDLE;
	this->deviceGraphicsQueueId = ur_uint(-1);
	this->deviceComputeQueueId = ur_uint(-1);
	this->deviceTransferQueueId = ur_uint(-1);
	this->vkGraphicsCommandPool = VK_NULL_HANDLE;
	this->vkComputeCommandPool = VK_NULL_HANDLE;
	this->vkTransferCommandPool = VK_NULL_HANDLE;
}

GrafDeviceVulkan::~GrafDeviceVulkan()
{
	this->Deinitialize();
}

Result GrafDeviceVulkan::Deinitialize()
{
	this->deviceGraphicsQueueId = ur_uint(-1);
	this->deviceComputeQueueId = ur_uint(-1);
	this->deviceTransferQueueId = ur_uint(-1);

	if (this->vkGraphicsCommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(this->vkDevice, this->vkGraphicsCommandPool, ur_null);
		this->vkGraphicsCommandPool = VK_NULL_HANDLE;
	}

	if (this->vkComputeCommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(this->vkDevice, this->vkComputeCommandPool, ur_null);
		this->vkComputeCommandPool = VK_NULL_HANDLE;
	}

	if (this->vkTransferCommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(this->vkDevice, this->vkTransferCommandPool, ur_null);
		this->vkTransferCommandPool = VK_NULL_HANDLE;
	}

	if (this->vkDevice != VK_NULL_HANDLE)
	{
		#if (UR_GRAF_VULKAN_IMPLICIT_WAIT_DEVICE)
		this->WaitIdle();
		#endif
		vkDestroyDevice(this->vkDevice, ur_null);
		this->vkDevice = VK_NULL_HANDLE;
		LogNoteGrafDbg("GrafDeviceVulkan: deinitialized");
	}

	return Result(Success);
}

Result GrafDeviceVulkan::Initialize(ur_uint deviceId)
{
	this->Deinitialize();

	LogNoteGrafDbg("GrafDeviceVulkan: initialization...");

	GrafDevice::Initialize(deviceId);

	// get corresponding physical device 

	GrafSystemVulkan& grafSystemVulkan = static_cast<GrafSystemVulkan&>(this->GetGrafSystem());
	VkPhysicalDevice vkPhysicalDevice = grafSystemVulkan.GetVkPhysicalDevice(this->GetDeviceId());
	if (VK_NULL_HANDLE == vkPhysicalDevice)
	{
		return ResultError(Failure, std::string("GrafDeviceVulkan: can not find VkPhysicalDevice for deviceId = ") + std::to_string(this->GetDeviceId()));
	}

	// enumerate device queue families

	ur_uint32 vkQueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &vkQueueFamilyCount, ur_null);
	std::vector<VkQueueFamilyProperties> vkQueueProperties(vkQueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &vkQueueFamilyCount, vkQueueProperties.data());
	ur_uint firstGraphicsQueueFamily = ur_uint(-1);
	ur_uint firstComputeQueueFamily = ur_uint(-1);
	ur_uint firstTransferQueueFamily = ur_uint(-1);
	for (ur_size i = 0; i < vkQueueProperties.size(); ++i)
	{
		if (VK_QUEUE_GRAPHICS_BIT & vkQueueProperties[i].queueFlags)
			firstGraphicsQueueFamily = std::min(ur_uint(i), firstGraphicsQueueFamily);
		if (VK_QUEUE_COMPUTE_BIT & vkQueueProperties[i].queueFlags)
			firstComputeQueueFamily = std::min(ur_uint(i), firstComputeQueueFamily);
		if (VK_QUEUE_TRANSFER_BIT & vkQueueProperties[i].queueFlags)
			firstTransferQueueFamily = std::min(ur_uint(i), firstTransferQueueFamily);
	}
	if (firstGraphicsQueueFamily == ur_uint(-1))
	{
		return ResultError(Failure, std::string("GrafDeviceVulkan: graphics queue family is not available for device Id = ") + std::to_string(this->GetDeviceId()));
	}
	this->deviceGraphicsQueueId = firstGraphicsQueueFamily;
	this->deviceComputeQueueId = firstComputeQueueFamily;
	this->deviceTransferQueueId = firstTransferQueueFamily;

	// create logical device

	VkDeviceQueueCreateInfo vkQueueInfo = {};
	vkQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	vkQueueInfo.flags = 0;
	vkQueueInfo.queueFamilyIndex = (ur_uint32)firstGraphicsQueueFamily;
	vkQueueInfo.queueCount = 1;
	ur_float queuePriority = 1.0f;
	vkQueueInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures vkDeviceFeatures = {};
	// NOTE: pass to Initialize a GRAF structure describing essential user defined feature list and fill corresponding fields in VkPhysicalDeviceFeatures

	VkDeviceCreateInfo vkDeviceInfo = {};
	vkDeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	vkDeviceInfo.flags = 0;
	vkDeviceInfo.queueCreateInfoCount = 1;
	vkDeviceInfo.pQueueCreateInfos = &vkQueueInfo;
	vkDeviceInfo.enabledLayerCount = (VulkanLayers ? ur_array_size(VulkanLayers) : 0);
	vkDeviceInfo.ppEnabledLayerNames = VulkanLayers;
	vkDeviceInfo.enabledExtensionCount = (VulkanDeviceExtensions ? ur_array_size(VulkanDeviceExtensions) : 0);
	vkDeviceInfo.ppEnabledExtensionNames = VulkanDeviceExtensions;
	vkDeviceInfo.pEnabledFeatures = &vkDeviceFeatures;

	VkResult res = vkCreateDevice(vkPhysicalDevice, &vkDeviceInfo, ur_null, &this->vkDevice);
	if (res != VK_SUCCESS)
	{
		return ResultError(Failure, std::string("GrafDeviceVulkan: vkCreateDevice failed with VkResult = ") + VkResultToString(res));
	}
	LogNoteGrafDbg(std::string("GrafDeviceVulkan: VkDevice created for ") + grafSystemVulkan.GetPhysicalDeviceDesc(deviceId)->Description);

	// create graphics queue command pool

	VkCommandPoolCreateInfo vkCommandPoolInfo = {};
	vkCommandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	vkCommandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // command buffers will be resseted implicitly at vkBeginCommandBuffer
	vkCommandPoolInfo.queueFamilyIndex = this->deviceGraphicsQueueId;

	res = vkCreateCommandPool(this->vkDevice, &vkCommandPoolInfo, ur_null, &this->vkGraphicsCommandPool);
	if (res != VK_SUCCESS)
	{
		this->Deinitialize();
		return ResultError(Failure, std::string("GrafDeviceVulkan: vkCreateCommandPool failed with VkResult = ") + VkResultToString(res));
	}
	LogNoteGrafDbg(std::string("GrafDeviceVulkan: VkCommandPool created for queue family Id = ") + std::to_string(this->deviceGraphicsQueueId));

	// NOTE: consider initializing command pools for other queue types when required

	return Result(Success);
}

Result GrafDeviceVulkan::Submit(GrafCommandList* grafCommandList)
{
	if (ur_null == grafCommandList)
		return Result(InvalidArgs);

	GrafCommandListVulkan* grafCommandListVulkan = static_cast<GrafCommandListVulkan*>(grafCommandList);
	VkCommandBuffer vkCommandBuffer = grafCommandListVulkan->GetVkCommandBuffer();
	
	// NOTE: support submission to different queue families
	// currently everything's done on the graphics queue
	VkQueue vkSubmissionQueue;
	vkGetDeviceQueue(this->vkDevice, this->deviceGraphicsQueueId, 0, &vkSubmissionQueue);

	VkSubmitInfo vkSubmitInfo = {};
	vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	vkSubmitInfo.waitSemaphoreCount = 0;
	vkSubmitInfo.pWaitSemaphores = ur_null;
    vkSubmitInfo.pWaitDstStageMask = ur_null;
	vkSubmitInfo.commandBufferCount = 1;
	vkSubmitInfo.pCommandBuffers = &vkCommandBuffer;
	vkSubmitInfo.signalSemaphoreCount = 0;
	vkSubmitInfo.pSignalSemaphores = ur_null;

	VkResult vkRes = vkQueueSubmit(vkSubmissionQueue, 1, &vkSubmitInfo, grafCommandListVulkan->GetVkSubmitFence());
	if (vkRes != VK_SUCCESS)
		return ResultError(Failure, std::string("GrafDeviceVulkan: vkQueueSubmit failed with VkResult = ") + VkResultToString(vkRes));

	return Result(Success);
}

Result GrafDeviceVulkan::WaitIdle()
{
	VkResult res = vkDeviceWaitIdle(this->vkDevice);
	if (res != VK_SUCCESS)
		return ResultError(Failure, std::string("GrafDeviceVulkan: vkDeviceWaitIdle failed with VkResult = ") + VkResultToString(res));
	return Result(Success);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GrafCommandListVulkan::GrafCommandListVulkan(GrafSystem &grafSystem) :
	GrafCommandList(grafSystem)
{
	this->vkCommandBuffer = VK_NULL_HANDLE;
	this->vkSubmitFence = VK_NULL_HANDLE;
}

GrafCommandListVulkan::~GrafCommandListVulkan()
{
	this->Deinitialize();
}

Result GrafCommandListVulkan::Deinitialize()
{
	if (this->vkSubmitFence != VK_NULL_HANDLE)
	{
		#if (UR_GRAF_VULKAN_DESTROY_WAIT_IMMEDIATE)
		vkWaitForFences(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), 1, &this->vkSubmitFence, true, ~ur_uint64(0));
		#endif
		vkDestroyFence(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkSubmitFence, ur_null);
		this->vkSubmitFence = VK_NULL_HANDLE;
	}

	if (this->vkCommandBuffer != VK_NULL_HANDLE)
	{
		GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice());
		vkFreeCommandBuffers(grafDeviceVulkan->GetVkDevice(), grafDeviceVulkan->GetVkGraphicsCommandPool(), 1, &this->vkCommandBuffer);
		this->vkCommandBuffer = VK_NULL_HANDLE;
	}

	return Result(Success);
}

Result GrafCommandListVulkan::Initialize(GrafDevice *grafDevice)
{
	this->Deinitialize();
	
	GrafCommandList::Initialize(grafDevice);

	// validate logical device 

	GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
	if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
	{
		return ResultError(InvalidArgs, std::string("GrafCommandListVulkan: failed to initialize, invalid GrafDevice"));
	}
	VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

	// allocate command bufer

	VkCommandBufferAllocateInfo vkCommandBufferInfo = {};
	vkCommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	vkCommandBufferInfo.commandPool = grafDeviceVulkan->GetVkGraphicsCommandPool();
	vkCommandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	vkCommandBufferInfo.commandBufferCount = 1;

	VkResult vkRes = vkAllocateCommandBuffers(vkDevice, &vkCommandBufferInfo, &this->vkCommandBuffer);
	if (vkRes != VK_SUCCESS)
		return ResultError(Failure, std::string("GrafCommandListVulkan: vkAllocateCommandBuffers failed with VkResult = ") + VkResultToString(vkRes));

	// submission sync object
	
	VkFenceCreateInfo vkFenceInfo = {};
	vkFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	vkFenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	vkRes = vkCreateFence(vkDevice, &vkFenceInfo, ur_null, &this->vkSubmitFence);
	if (vkRes != VK_SUCCESS)
	{
		this->Deinitialize();
		return ResultError(Failure, std::string("GrafCommandListVulkan: vkCreateFence failed with VkResult = ") + VkResultToString(vkRes));
	}

	return Result(Success);
}

Result GrafCommandListVulkan::Begin()
{
	if (VK_NULL_HANDLE == this->vkCommandBuffer)
		return Result(NotInitialized);

	VkDevice vkDevice = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice();
	vkWaitForFences(vkDevice, 1, &this->vkSubmitFence, true, ~ur_uint64(0)); // make sure command buffer is no longer used (previous submission can still be executed)
	vkResetFences(vkDevice, 1, &this->vkSubmitFence);

	VkCommandBufferBeginInfo vkBeginInfo = {};
	vkBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkResult vkRes = vkBeginCommandBuffer(this->vkCommandBuffer, &vkBeginInfo);
	if (vkRes != VK_SUCCESS)
		return ResultError(Failure, std::string("GrafCommandListVulkan: vkBeginCommandBuffer failed with VkResult = ") + VkResultToString(vkRes));

	return Result(Success);
}

Result GrafCommandListVulkan::End()
{
	if (VK_NULL_HANDLE == this->vkCommandBuffer)
		return Result(NotInitialized);

	VkResult vkRes = vkEndCommandBuffer(this->vkCommandBuffer);
	if (vkRes != VK_SUCCESS)
		return ResultError(Failure, std::string("GrafCommandListVulkan: vkEndCommandBuffer failed with VkResult = ") + VkResultToString(vkRes));

	return Result(Success);
}

Result GrafCommandListVulkan::ImageMemoryBarrier(GrafImage* grafImage, GrafImageState srcState, GrafImageState dstState)
{
	if (ur_null == grafImage)
		return Result(InvalidArgs);

	if (grafImage->GetState() == dstState)
		return Result(Success);

	VkImageMemoryBarrier vkImageBarrier = {};
	vkImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	vkImageBarrier.srcAccessMask = 0;
	vkImageBarrier.dstAccessMask = 0;
	vkImageBarrier.oldLayout = GrafUtilsVulkan::GrafToVkImageLayout(GrafImageState::Current == srcState ? grafImage->GetState() : srcState);
	vkImageBarrier.newLayout = GrafUtilsVulkan::GrafToVkImageLayout(dstState);
	vkImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	vkImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	vkImageBarrier.image = static_cast<GrafImageVulkan*>(grafImage)->GetVkImage();
	vkImageBarrier.subresourceRange.aspectMask = GrafUtilsVulkan::GrafToVkImageUsageAspect(grafImage->GetDesc().Usage);
	vkImageBarrier.subresourceRange.baseMipLevel = 0;
	vkImageBarrier.subresourceRange.levelCount = (ur_uint32)grafImage->GetDesc().MipLevels;
	vkImageBarrier.subresourceRange.baseArrayLayer = 0;
	vkImageBarrier.subresourceRange.layerCount = 1;

	// deduce stage mask values from source and destination layouts
	VkPipelineStageFlags vkStageSrc = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkPipelineStageFlags vkStageDst = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	
	vkCmdPipelineBarrier(this->vkCommandBuffer, vkStageSrc, vkStageDst, VkDependencyFlags(0), 0, ur_null, 0, ur_null, 1, &vkImageBarrier);

	static_cast<GrafImageVulkan*>(grafImage)->SetState(dstState);
	
	return Result(Success);
}

Result GrafCommandListVulkan::SetFenceState(GrafFence* grafFence, GrafFenceState state)
{
	if (ur_null == grafFence)
		return Result(InvalidArgs);

	VkPipelineStageFlags vkPipelineStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkEvent vkFenceEvent = static_cast<GrafFenceVulkan*>(grafFence)->GetVkEvent();
	switch (state)
	{
	case GrafFenceState::Signaled:
		vkCmdSetEvent(this->vkCommandBuffer, vkFenceEvent, vkPipelineStage);
		break;
	case GrafFenceState::Reset:
		vkCmdResetEvent(this->vkCommandBuffer, vkFenceEvent, vkPipelineStage);
		break;
	default:
		return ResultError(InvalidArgs, "GrafCommandListVulkan: SetFenceState failed, invalid GrafFenceState");
	};

	return Result(Success);
}

Result GrafCommandListVulkan::ClearColorImage(GrafImage* grafImage, GrafClearValue clearValue)
{
	if (ur_null == grafImage)
		return Result(InvalidArgs);

	VkImage vkImage = static_cast<GrafImageVulkan*>(grafImage)->GetVkImage();
	VkImageLayout vkImageLayout = GrafUtilsVulkan::GrafToVkImageLayout(grafImage->GetState());
	VkClearColorValue& vkClearValue = reinterpret_cast<VkClearColorValue&>(clearValue);
	VkImageSubresourceRange vkClearRange = {};
	vkClearRange.aspectMask = GrafUtilsVulkan::GrafToVkImageUsageAspect(grafImage->GetDesc().Usage);
	vkClearRange.levelCount = grafImage->GetDesc().MipLevels;
	vkClearRange.baseArrayLayer = 0;
	vkClearRange.layerCount = 1;
	
	vkCmdClearColorImage(this->vkCommandBuffer, vkImage, vkImageLayout, &vkClearValue, 1, &vkClearRange);

	return Result(Success);
}

Result GrafCommandListVulkan::SetViewport(const GrafViewportDesc& grafViewportDesc, ur_bool resetScissorsRect)
{
	VkViewport vkViewport = {};
	vkViewport.x = grafViewportDesc.X;
	vkViewport.y = grafViewportDesc.Y;
	vkViewport.width = grafViewportDesc.Width;
	vkViewport.height = grafViewportDesc.Height;
	vkViewport.minDepth = grafViewportDesc.Near;
	vkViewport.maxDepth = grafViewportDesc.Far;
	
	vkCmdSetViewport(vkCommandBuffer, 0, 1, &vkViewport);

	if (resetScissorsRect)
	{
		RectI scissorsRect;
		scissorsRect.Min.x = ur_int(grafViewportDesc.X);
		scissorsRect.Min.y = ur_int(grafViewportDesc.Y);
		scissorsRect.Max.x = ur_int(grafViewportDesc.X + grafViewportDesc.Width);
		scissorsRect.Max.y = ur_int(grafViewportDesc.Y + grafViewportDesc.Height);

		return SetScissorsRect(scissorsRect);
	}

	return Result(Success);
}

Result GrafCommandListVulkan::SetScissorsRect(const RectI& scissorsRect)
{
	if (scissorsRect.IsInsideOut())
		return Result(InvalidArgs);

	VkRect2D vkScissors;
	vkScissors.offset.x = (ur_int32)scissorsRect.Min.x;
	vkScissors.offset.y = (ur_int32)scissorsRect.Min.y;
	vkScissors.extent.width = scissorsRect.Width();
	vkScissors.extent.height = scissorsRect.Height();
	
	vkCmdSetScissor(vkCommandBuffer, 0, 1, &vkScissors);

	return Result(Success);
}

Result GrafCommandListVulkan::BeginRenderPass(GrafRenderPass* grafRenderPass, GrafRenderTarget* grafRenderTarget)
{
	if (ur_null == grafRenderPass || ur_null == grafRenderTarget || 0 == grafRenderTarget->GetImageCount())
		return Result(InvalidArgs);

	VkRenderPassBeginInfo vkPassBeginInfo = {};
	vkPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	vkPassBeginInfo.renderPass = static_cast<GrafRenderPassVulkan*>(grafRenderPass)->GetVkRenderPass();
	vkPassBeginInfo.framebuffer = static_cast<GrafRenderTargetVulkan*>(grafRenderTarget)->GetVkFramebuffer();
	vkPassBeginInfo.renderArea.offset = { 0, 0 };
	vkPassBeginInfo.renderArea.extent.width = (ur_uint32)grafRenderTarget->GetImage(0)->GetDesc().Size.x;
	vkPassBeginInfo.renderArea.extent.height = (ur_uint32)grafRenderTarget->GetImage(0)->GetDesc().Size.y;
	vkPassBeginInfo.clearValueCount = 0;
	vkPassBeginInfo.pClearValues = ur_null;

	vkCmdBeginRenderPass(this->vkCommandBuffer, &vkPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	return Result(Success);
}

Result GrafCommandListVulkan::EndRenderPass()
{
	vkCmdEndRenderPass(this->vkCommandBuffer);

	return Result(Success);
}

Result GrafCommandListVulkan::BindPipeline(GrafPipeline* grafPipeline)
{
	if (ur_null == grafPipeline)
		return Result(InvalidArgs);

	vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, static_cast<GrafPipelineVulkan*>(grafPipeline)->GetVkPipeline());

	return Result(Success);
}

Result GrafCommandListVulkan::BindVertexBuffer(GrafBuffer* grafVertexBuffer, ur_uint bindingIdx)
{
	if (ur_null == grafVertexBuffer)
		return Result(InvalidArgs);

	VkBuffer vkBuffers[] = { static_cast<GrafBufferVulkan*>(grafVertexBuffer)->GetVkBuffer() };
	VkDeviceSize vkOffsets[] = { 0 };
	vkCmdBindVertexBuffers(vkCommandBuffer, bindingIdx, 1, vkBuffers, vkOffsets);

	return Result(Success);
}

Result GrafCommandListVulkan::Draw(ur_uint vertexCount, ur_uint instanceCount, ur_uint firstVertex, ur_uint firstInstance)
{
	vkCmdDraw(this->vkCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);

	return Result(Success);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GrafFenceVulkan::GrafFenceVulkan(GrafSystem &grafSystem) :
	GrafFence(grafSystem)
{
	this->vkEvent = VK_NULL_HANDLE;
}

GrafFenceVulkan::~GrafFenceVulkan()
{
	this->Deinitialize();
}

Result GrafFenceVulkan::Deinitialize()
{
	if (this->vkEvent != VK_NULL_HANDLE)
	{
		vkDestroyEvent(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkEvent, ur_null);
		this->vkEvent = VK_NULL_HANDLE;
	}

	return Result(Success);
}

Result GrafFenceVulkan::Initialize(GrafDevice *grafDevice)
{
	this->Deinitialize();

	GrafFence::Initialize(grafDevice);

	// validate logical device 

	GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
	if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
	{
		return ResultError(InvalidArgs, std::string("GrafFenceVulkan: failed to initialize, invalid GrafDevice"));
	}
	VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

	// create synchronization object

	VkEventCreateInfo vkEventInfo = {};
	vkEventInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
	vkEventInfo.flags = 0;

	VkResult vkRes = vkCreateEvent(vkDevice, &vkEventInfo, ur_null, &this->vkEvent);
	if (vkRes != VK_SUCCESS)
	{
		return ResultError(Failure, std::string("GrafFenceVulkan: vkCreateEvent failed with VkResult = ") + VkResultToString(vkRes));
	}

	return Result(Success);
}

Result GrafFenceVulkan::SetState(GrafFenceState state)
{
	VkResult vkRes = VK_SUCCESS;
	
	switch (state)
	{
	case GrafFenceState::Reset:
		vkRes = vkResetEvent(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkEvent);
		break;
	case GrafFenceState::Signaled:
		vkRes = vkSetEvent(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkEvent);
		break;
	default:
		return ResultError(InvalidArgs, "GrafFenceVulkan: SetState failed, invalid GrafFenceState");
	};
	
	if (vkRes != VK_SUCCESS)
		return ResultError(Failure, std::string("GrafFenceVulkan: vkResetEvent failed with VkResult = ") + VkResultToString(vkRes));
	
	return Result(Success);
}

Result GrafFenceVulkan::WaitSignaled()
{
	Result res(Success);
	
	GrafFenceState state;
	do
	{
		res = this->GetState(state);
	} while (state != GrafFenceState::Signaled && !Failed(res));
	
	return res;
}

Result GrafFenceVulkan::GetState(GrafFenceState& state)
{
	VkResult vkRes = vkGetEventStatus(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkEvent);
	if (VK_EVENT_SET == vkRes)
		state = GrafFenceState::Signaled;
	else if (VK_EVENT_RESET == vkRes)
		state = GrafFenceState::Reset;
	else
		return ResultError(Failure, std::string("GrafFenceVulkan: vkGetEventStatus failed with VkResult = ") + VkResultToString(vkRes));
	return Result(Success);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GrafCanvasVulkan::GrafCanvasVulkan(GrafSystem &grafSystem) :
	GrafCanvas(grafSystem)
{
	this->vkSurface = VK_NULL_HANDLE;
	this->vkSwapChain = VK_NULL_HANDLE;
	this->vkDevicePresentQueueId = 0;
	this->frameCount = 0;
	this->frameIdx = 0;
}

GrafCanvasVulkan::~GrafCanvasVulkan()
{
	this->Deinitialize();
}

Result GrafCanvasVulkan::Deinitialize()
{
	this->frameCount = 0;
	this->frameIdx = 0;
	this->swapChainImageCount = 0;
	this->swapChainCurrentImageId = 0;

	this->imageTransitionCmdListBegin.clear();
	this->imageTransitionCmdListEnd.clear();

	for (auto& vkSemaphore : this->vkSemaphoreImageAcquired)
	{
		if (vkSemaphore != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), vkSemaphore, ur_null);
		}
	}
	this->vkSemaphoreImageAcquired.clear();

	for (auto& vkSemaphore : this->vkSemaphoreRenderFinished)
	{
		if (vkSemaphore != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), vkSemaphore, ur_null);
		}
	}
	this->vkSemaphoreRenderFinished.clear();
	
	this->swapChainImages.clear();

	if (this->vkSwapChain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkSwapChain, ur_null);
		this->vkSwapChain = VK_NULL_HANDLE;
		LogNoteGrafDbg("GrafCanvasVulkan: swap chain destroyed");
	}
	
	if (this->vkSurface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(static_cast<GrafSystemVulkan&>(this->GetGrafSystem()).GetVkInstance(), this->vkSurface, ur_null);
		this->vkSurface = VK_NULL_HANDLE;
		LogNoteGrafDbg("GrafCanvasVulkan: surface destroyed");
	}

	return Result(Success);
}

Result GrafCanvasVulkan::Initialize(GrafDevice* grafDevice, const InitParams& initParams)
{
	this->Deinitialize();

	LogNoteGrafDbg("GrafCanvasVulkan: initialization...");

	GrafCanvas::Initialize(grafDevice, initParams);

	// validate logical device 

	GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
	if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
	{
		return ResultError(InvalidArgs, std::string("GrafCanvasVulkan: failed to initialize, invalid GrafDevice"));
	}
	VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

	// create surface

	GrafSystemVulkan& grafSystemVulkan = static_cast<GrafSystemVulkan&>(this->GetGrafSystem());

	#if defined(_WINDOWS)

	WinCanvas* winCanvas = static_cast<WinCanvas*>(this->GetRealm().GetCanvas());
	if (ur_null == winCanvas)
	{
		return ResultError(InvalidArgs, std::string("GrafCanvasVulkan: failed to initialize, invalid WinCanvas"));
	}

	VkWin32SurfaceCreateInfoKHR vkSurfaceInfo = {};
	vkSurfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	vkSurfaceInfo.flags = 0;
	vkSurfaceInfo.hinstance = winCanvas->GetHinstance();
	vkSurfaceInfo.hwnd = winCanvas->GetHwnd();

	VkResult res = vkCreateWin32SurfaceKHR(grafSystemVulkan.GetVkInstance(), &vkSurfaceInfo, ur_null, &this->vkSurface);
	if (res != VK_SUCCESS)
	{
		return ResultError(Failure, std::string("GrafCanvasVulkan: vkCreateWin32SurfaceKHR failed with VkResult = ") + VkResultToString(res));
	}
	LogNoteGrafDbg("GrafCanvasVulkan: VkSurfaceKHR created");

	#else

	return ResultError(NotImplemented, std::string("GrafCanvasVulkan: failed to initialize, unsupported platform"));

	#endif

	// validate presentation support on current device

	VkPhysicalDevice vkPhysicalDevice = grafSystemVulkan.GetVkPhysicalDevice(grafDeviceVulkan->GetDeviceId());
	ur_uint32 vkDeviceGraphicsQueueId = (ur_uint32)grafDeviceVulkan->GetVkDeviceGraphicsQueueId();
	VkBool32 presentationSupported = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, vkDeviceGraphicsQueueId, this->vkSurface, &presentationSupported);
	if (!presentationSupported)
	{
		this->Deinitialize();
		return ResultError(Failure, "GrafCanvasVulkan: failed to initialize, selected physical device graphics queue does not support presentation");
	}
	this->vkDevicePresentQueueId = vkDeviceGraphicsQueueId;
	const ur_uint32 vkDevicePresentQueueCount = 1;
	ur_uint32 vkDevicePresentQueueIds[vkDevicePresentQueueCount] = { this->vkDevicePresentQueueId };

	// validate swap chain caps

	VkSurfaceCapabilitiesKHR vkSurfaceCaps = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, this->vkSurface, &vkSurfaceCaps);

	VkSurfaceFormatKHR vkSurfaceFormat = {};
	ur_uint32 vkSurfaceFormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, this->vkSurface, &vkSurfaceFormatCount, ur_null);
	std::vector<VkSurfaceFormatKHR> vkSurfaceFormats(vkSurfaceFormatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, this->vkSurface, &vkSurfaceFormatCount, vkSurfaceFormats.data());
	ur_bool surfaceFormatFound = false;
	VkFormat vkFormatRequested = GrafUtilsVulkan::GrafToVkFormat(initParams.Format);
	for (ur_size i = 0; i < vkSurfaceFormats.size(); ++i)
	{
		if (vkFormatRequested == vkSurfaceFormats[i].format)
			surfaceFormatFound = true;
		if (surfaceFormatFound)
		{
			vkSurfaceFormat = vkSurfaceFormats[i];
			break;
		}
	}
	if (!surfaceFormatFound)
	{
		LogWarning("GrafCanvasVulkan: requested format is not supported, switching to the first available");
		if (!vkSurfaceFormats.empty())
		{
			vkSurfaceFormat = vkSurfaceFormats[0];
			surfaceFormatFound = true;
		}
	}
	if (!surfaceFormatFound)
	{
		this->Deinitialize();
		return ResultError(Failure, "GrafCanvasVulkan: failed to initialize, no surface format supported");
	}

	VkPresentModeKHR vkPresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
	ur_uint32 vkPresentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, this->vkSurface, &vkPresentModeCount, ur_null);
	std::vector<VkPresentModeKHR> vkPresentModes(vkPresentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, this->vkSurface, &vkPresentModeCount, vkPresentModes.data());
	ur_bool presentModeFound = false;
	for (ur_size i = 0; i < vkPresentModes.size(); ++i)
	{
		if (GrafPresentMode::Immediate == initParams.PresentMode &&
			VK_PRESENT_MODE_IMMEDIATE_KHR == vkPresentModes[i])
			presentModeFound = true;
		else if (GrafPresentMode::VerticalSync == initParams.PresentMode &&
			VK_PRESENT_MODE_FIFO_KHR == vkPresentModes[i])
			presentModeFound = true;
		if (presentModeFound)
		{
			vkPresentMode = vkPresentModes[i];
			break;
		}
	}
	if (!presentModeFound)
	{
		LogWarning("GrafCanvasVulkan: requested presentation mode is not supported, switching to the first available");
		if (!vkPresentModes.empty())
		{
			vkPresentMode = vkPresentModes[0];
			presentModeFound = true;
		}
	}
	if (!presentModeFound)
	{
		this->Deinitialize();
		return ResultError(Failure, "GrafCanvasVulkan: failed to initialize, no presentation mode supported");
	}
	
	VkExtent2D vkSurfaceExtent = {};
	if (vkSurfaceCaps.currentExtent.width != ur_uint32(-1))
	{
		// only this extent can be used
		vkSurfaceExtent = vkSurfaceCaps.currentExtent;
	}
	else
	{
		// extent between reported min/max can be used, we choose the maximal
		vkSurfaceExtent = vkSurfaceCaps.maxImageExtent;
	}

	ur_uint32 vkSwapChainImageCount = (ur_uint32)initParams.SwapChainImageCount;
	vkSwapChainImageCount = std::max(vkSurfaceCaps.minImageCount, vkSwapChainImageCount);
	vkSwapChainImageCount = std::min(vkSurfaceCaps.maxImageCount, vkSwapChainImageCount);

	// create swap chain

	VkSwapchainCreateInfoKHR vkSwapChainInfo = {};
	vkSwapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	vkSwapChainInfo.flags = 0;
	vkSwapChainInfo.surface = this->vkSurface;
	vkSwapChainInfo.minImageCount = vkSwapChainImageCount;
	vkSwapChainInfo.imageFormat = vkSurfaceFormat.format;
	vkSwapChainInfo.imageColorSpace = vkSurfaceFormat.colorSpace;
	vkSwapChainInfo.imageExtent = vkSurfaceExtent;
	vkSwapChainInfo.imageArrayLayers = 1;
	vkSwapChainInfo.imageUsage = (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	vkSwapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // the same graphics queue is used (vkDeviceGraphicsQueueId)
	vkSwapChainInfo.queueFamilyIndexCount = vkDevicePresentQueueCount;
	vkSwapChainInfo.pQueueFamilyIndices = vkDevicePresentQueueIds;
	vkSwapChainInfo.preTransform = vkSurfaceCaps.currentTransform;
	vkSwapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	vkSwapChainInfo.presentMode = vkPresentMode;
	vkSwapChainInfo.clipped = VK_TRUE;
	vkSwapChainInfo.oldSwapchain = VK_NULL_HANDLE;

	res = vkCreateSwapchainKHR(vkDevice, &vkSwapChainInfo, ur_null, &this->vkSwapChain);
	if (res != VK_SUCCESS)
	{
		this->Deinitialize();
		return ResultError(Failure, std::string("GrafCanvasVulkan: vkCreateSwapchainKHR failed with VkResult = ") + VkResultToString(res));
	}
	LogNoteGrafDbg("GrafCanvasVulkan: VkSwapchainKHR created");

	// retrieve swap chain images

	ur_uint32 vkSwapChainImageRealCount = 0;
	vkGetSwapchainImagesKHR(vkDevice, this->vkSwapChain, &vkSwapChainImageRealCount, ur_null);
	std::vector<VkImage> vkSwapChainImages(vkSwapChainImageRealCount);
	vkGetSwapchainImagesKHR(vkDevice, this->vkSwapChain, &vkSwapChainImageRealCount, vkSwapChainImages.data());
	if (0 == vkSwapChainImageRealCount)
	{
		this->Deinitialize();
		return ResultError(Failure, "GrafCanvasVulkan: vkGetSwapchainImagesKHR failed with zero images returned");
	}
	this->swapChainImageCount = vkSwapChainImageRealCount;
	this->swapChainCurrentImageId = 0;

	this->swapChainImages.reserve(vkSwapChainImages.size());
	for (VkImage& vkImage : vkSwapChainImages)
	{
		GrafImage::InitParams imageParams = {};
		imageParams.ImageDesc.Type = GrafImageType::Tex2D;
		imageParams.ImageDesc.Format = GrafUtilsVulkan::VkToGrafFormat(vkSwapChainInfo.imageFormat);
		imageParams.ImageDesc.Size.x = vkSwapChainInfo.imageExtent.width;
		imageParams.ImageDesc.Size.y = vkSwapChainInfo.imageExtent.height;
		imageParams.ImageDesc.Size.z = 0;
		imageParams.ImageDesc.MipLevels = 1;
		imageParams.ImageDesc.Usage = GrafUtilsVulkan::VkToGrafImageUsage(vkSwapChainInfo.imageUsage);
		
		std::unique_ptr<GrafImage> grafImage;
		grafSystemVulkan.CreateImage(grafImage);
		Result res = static_cast<GrafImageVulkan*>(grafImage.get())->InitializeFromVkImage(grafDevice, imageParams, vkImage);
		if (Failed(res))
		{
			this->Deinitialize();
			return ResultError(Failure, "GrafCanvasVulkan: failed to create swap chain images");
		}
		this->swapChainImages.push_back(std::move(grafImage));
	}

	// per frame objects

	this->frameCount = vkSwapChainImageRealCount; // command list(s) and sync object(s) per image
	this->frameIdx = ur_uint32(this->frameCount - 1); // first AcquireNextImage will make it 0

	// create presentation sync objects 

	VkSemaphoreCreateInfo vkSemaphoreInfo = {};
	vkSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkSemaphoreInfo.flags = 0;

	this->vkSemaphoreImageAcquired.reserve(this->frameCount);
	for (ur_uint32 iframe = 0; iframe < this->frameCount; ++iframe)
	{
		VkSemaphore vkSemaphore;
		res = vkCreateSemaphore(vkDevice, &vkSemaphoreInfo, ur_null, &vkSemaphore);
		if (res != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafCanvasVulkan: vkCreateSemaphore failed with VkResult = ") + VkResultToString(res));
		}
		this->vkSemaphoreImageAcquired.push_back(vkSemaphore);
	}

	this->vkSemaphoreRenderFinished.reserve(this->frameCount);
	for (ur_uint32 iframe = 0; iframe < this->frameCount; ++iframe)
	{
		VkSemaphore vkSemaphore;
		res = vkCreateSemaphore(vkDevice, &vkSemaphoreInfo, ur_null, &vkSemaphore);
		if (res != VK_SUCCESS)
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafCanvasVulkan: vkCreateSemaphore failed with VkResult = ") + VkResultToString(res));
		}
		this->vkSemaphoreRenderFinished.push_back(vkSemaphore);
	}

	// create comamnd list for image state/layout transitions

	this->imageTransitionCmdListBegin.resize(this->frameCount);
	this->imageTransitionCmdListEnd.resize(this->frameCount);
	for (ur_uint32 iframe = 0; iframe < this->frameCount; ++iframe)
	{
		Result urRes = Success;
		urRes &= this->GetGrafSystem().CreateCommandList(this->imageTransitionCmdListBegin[iframe]);
		urRes &= this->GetGrafSystem().CreateCommandList(this->imageTransitionCmdListEnd[iframe]);
		if (Succeeded(urRes))
		{
			urRes &= this->imageTransitionCmdListBegin[iframe]->Initialize(grafDevice);
			urRes &= this->imageTransitionCmdListEnd[iframe]->Initialize(grafDevice);
		}
		if (Failed(urRes))
		{
			this->Deinitialize();
			return ResultError(Failure, "GrafCanvasVulkan: failed to create transition command list");
		}
	}

	// acquire an image to use as a current RT
	
	Result urRes = this->AcquireNextImage();
	if (Failed(urRes))
	{
		this->Deinitialize();
		return urRes;
	}

	// initial image layout transition into common state

	#if (UR_GRAF_VULKAN_SWAPCHAIN_NEXT_IMAGE_IMPLICIT_TRANSITION_TO_GENERAL)
	GrafImage* swapChainNextImage = this->swapChainImages[this->swapChainCurrentImageId].get();
	GrafCommandList* imageTransitionCmdListBeginCrnt = this->imageTransitionCmdListBegin[this->frameIdx].get();
	imageTransitionCmdListBeginCrnt->Begin();
	imageTransitionCmdListBeginCrnt->ImageMemoryBarrier(swapChainNextImage, GrafImageState::Current, GrafImageState::Common);
	imageTransitionCmdListBeginCrnt->End();
	{
		// submit with semaphore (wait image's acquired before using it as render target)
		GrafCommandListVulkan* grafCommandListVulkan = static_cast<GrafCommandListVulkan*>(imageTransitionCmdListBeginCrnt);
		VkCommandBuffer vkCommandBuffer = grafCommandListVulkan->GetVkCommandBuffer();
		VkQueue vkSubmissionQueue;
		vkGetDeviceQueue(vkDevice, grafDeviceVulkan->GetVkDeviceGraphicsQueueId(), 0, &vkSubmissionQueue);
		VkPipelineStageFlags waitStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		VkSubmitInfo vkSubmitInfo = {};
		vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		vkSubmitInfo.waitSemaphoreCount = 1;
		vkSubmitInfo.pWaitSemaphores = &this->vkSemaphoreImageAcquired[this->frameIdx];
		vkSubmitInfo.pWaitDstStageMask = &waitStageFlags;
		vkSubmitInfo.commandBufferCount = 1;
		vkSubmitInfo.pCommandBuffers = &vkCommandBuffer;
		vkSubmitInfo.signalSemaphoreCount = 0;
		vkSubmitInfo.pSignalSemaphores = ur_null;
		VkResult vkRes = vkQueueSubmit(vkSubmissionQueue, 1, &vkSubmitInfo, grafCommandListVulkan->GetVkSubmitFence());
		if (vkRes != VK_SUCCESS)
			return ResultError(Failure, std::string("GrafDeviceVulkan: vkQueueSubmit failed with VkResult = ") + VkResultToString(vkRes));
	}
	#endif

	return Result(Success);
}

Result GrafCanvasVulkan::Present()
{
	GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice());
	VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();
	VkQueue vkDevicePresentQueue;
	vkGetDeviceQueue(vkDevice, this->vkDevicePresentQueueId, 0, &vkDevicePresentQueue);

	// do image layout transition to presentation state

	GrafImage* swapChainCurrentImage = this->swapChainImages[this->swapChainCurrentImageId].get();
	GrafCommandList* imageTransitionCmdListEndCrnt = this->imageTransitionCmdListEnd[this->frameIdx].get();
	imageTransitionCmdListEndCrnt->Begin();
	imageTransitionCmdListEndCrnt->ImageMemoryBarrier(swapChainCurrentImage, GrafImageState::Current, GrafImageState::Present);
	imageTransitionCmdListEndCrnt->End();
	{
		// submit with semaphore (signal rendering's finished before presenting )
		GrafCommandListVulkan* grafCommandListVulkan = static_cast<GrafCommandListVulkan*>(imageTransitionCmdListEndCrnt);
		VkCommandBuffer vkCommandBuffer = grafCommandListVulkan->GetVkCommandBuffer();
		VkQueue vkSubmissionQueue;
		vkGetDeviceQueue(vkDevice, grafDeviceVulkan->GetVkDeviceGraphicsQueueId(), 0, &vkSubmissionQueue);
		VkSubmitInfo vkSubmitInfo = {};
		vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		vkSubmitInfo.waitSemaphoreCount = 0;
		vkSubmitInfo.pWaitSemaphores = ur_null;
		vkSubmitInfo.pWaitDstStageMask = ur_null;
		vkSubmitInfo.commandBufferCount = 1;
		vkSubmitInfo.pCommandBuffers = &vkCommandBuffer;
		vkSubmitInfo.signalSemaphoreCount = 1;
		vkSubmitInfo.pSignalSemaphores = &this->vkSemaphoreRenderFinished[this->frameIdx];
		VkResult vkRes = vkQueueSubmit(vkSubmissionQueue, 1, &vkSubmitInfo, grafCommandListVulkan->GetVkSubmitFence());
		if (vkRes != VK_SUCCESS)
			return ResultError(Failure, std::string("GrafDeviceVulkan: vkQueueSubmit failed with VkResult = ") + VkResultToString(vkRes));
	}

	// present current image

	VkSwapchainKHR swapChains[] = { this->vkSwapChain };

	VkPresentInfoKHR vkPresentInfo = {};
	vkPresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	vkPresentInfo.waitSemaphoreCount = 1;
	vkPresentInfo.pWaitSemaphores = &this->vkSemaphoreRenderFinished[this->frameIdx]; // make sure rendering's finished before presenting
	vkPresentInfo.swapchainCount = ur_array_size(swapChains);
	vkPresentInfo.pSwapchains = swapChains;
	vkPresentInfo.pImageIndices = &this->swapChainCurrentImageId;
	vkPresentInfo.pResults = ur_null;

	VkResult vkRes = vkQueuePresentKHR(vkDevicePresentQueue, &vkPresentInfo);
	if (vkRes != VK_SUCCESS)
		return ResultError(Failure, std::string("GrafCanvasVulkan: vkQueuePresentKHR failed with VkResult = ") + VkResultToString(vkRes));
	
	// acquire next available image to use as RT

	Result res = this->AcquireNextImage();
	if (Failed(res))
		return res;

	// do image layout transition to common state
	
	#if (UR_GRAF_VULKAN_SWAPCHAIN_NEXT_IMAGE_IMPLICIT_TRANSITION_TO_GENERAL)
	GrafImage* swapChainNextImage = this->swapChainImages[this->swapChainCurrentImageId].get();
	GrafCommandList* imageTransitionCmdListBeginNext = this->imageTransitionCmdListBegin[this->frameIdx].get();
	imageTransitionCmdListBeginNext->Begin();
	imageTransitionCmdListBeginNext->ImageMemoryBarrier(swapChainNextImage, GrafImageState::Current, GrafImageState::Common);
	imageTransitionCmdListBeginNext->End();
	{
		// submit with semaphore (wait image's acquired before using it as render target)
		GrafCommandListVulkan* grafCommandListVulkan = static_cast<GrafCommandListVulkan*>(imageTransitionCmdListBeginNext);
		VkCommandBuffer vkCommandBuffer = grafCommandListVulkan->GetVkCommandBuffer();
		VkQueue vkSubmissionQueue;
		vkGetDeviceQueue(vkDevice, grafDeviceVulkan->GetVkDeviceGraphicsQueueId(), 0, &vkSubmissionQueue);
		VkPipelineStageFlags waitStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		VkSubmitInfo vkSubmitInfo = {};
		vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		vkSubmitInfo.waitSemaphoreCount = 1;
		vkSubmitInfo.pWaitSemaphores = &this->vkSemaphoreImageAcquired[this->frameIdx];
		vkSubmitInfo.pWaitDstStageMask = &waitStageFlags;
		vkSubmitInfo.commandBufferCount = 1;
		vkSubmitInfo.pCommandBuffers = &vkCommandBuffer;
		vkSubmitInfo.signalSemaphoreCount = 0;
		vkSubmitInfo.pSignalSemaphores = ur_null;
		VkResult vkRes = vkQueueSubmit(vkSubmissionQueue, 1, &vkSubmitInfo, grafCommandListVulkan->GetVkSubmitFence());
		if (vkRes != VK_SUCCESS)
			return ResultError(Failure, std::string("GrafDeviceVulkan: vkQueueSubmit failed with VkResult = ") + VkResultToString(vkRes));
	}
	#endif

	return Result(Success);
}

Result GrafCanvasVulkan::AcquireNextImage()
{
	GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice());
	VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

	this->frameIdx = (this->frameIdx + 1) % this->frameCount;

	VkResult res = vkAcquireNextImageKHR(vkDevice, this->vkSwapChain, ~ur_uint64(0), this->vkSemaphoreImageAcquired[this->frameIdx], VK_NULL_HANDLE, &this->swapChainCurrentImageId);
	if (res != VK_SUCCESS)
		return ResultError(Failure, std::string("GrafCanvasVulkan: vkAcquireNextImageKHR failed with VkResult = ") + VkResultToString(res));

	return Result(Success);
}

GrafImage* GrafCanvasVulkan::GetCurrentImage()
{
	return this->GetSwapChainImage((ur_uint)this->swapChainCurrentImageId);
}

GrafImage* GrafCanvasVulkan::GetSwapChainImage(ur_uint imageId)
{
	if ((ur_size)imageId > this->swapChainImages.size())
		return ur_null;
	return this->swapChainImages[imageId].get();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GrafImageVulkan::GrafImageVulkan(GrafSystem &grafSystem) :
	GrafImage(grafSystem)
{
	this->imageExternalHandle = false;
	this->vkImage = VK_NULL_HANDLE;
	this->vkImageView = VK_NULL_HANDLE;
}

GrafImageVulkan::~GrafImageVulkan()
{
	this->Deinitialize();
}

Result GrafImageVulkan::Deinitialize()
{
	if (this->vkImageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkImageView, ur_null);
		this->vkImageView = VK_NULL_HANDLE;
	}
	if (this->vkImage != VK_NULL_HANDLE && !this->imageExternalHandle)
	{
		vkDestroyImage(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkImage, ur_null);
	}
	this->vkImage = VK_NULL_HANDLE;
	this->imageExternalHandle = false;

	return Result(Success);
}

Result GrafImageVulkan::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
{
	this->Deinitialize();

	GrafImage::Initialize(grafDevice, initParams);
	
	// TODO

	// create views

	Result res = this->CreateVkImageViews();
	if (Failed(res))
		return res;
	
	return Result(NotImplemented);
}

Result GrafImageVulkan::InitializeFromVkImage(GrafDevice *grafDevice, const InitParams& initParams, VkImage vkImage)
{
	this->Deinitialize();

	GrafImage::Initialize(grafDevice, initParams);

	// validate logical device 

	GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
	if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
	{
		return ResultError(InvalidArgs, std::string("GrafImageVulkan: failed to initialize, invalid GrafDevice"));
	}

	// init external image

	this->imageExternalHandle = true;
	this->vkImage = vkImage;

	// create views

	if (this->vkImage != VK_NULL_HANDLE)
	{
		Result res = this->CreateVkImageViews();
		if (Failed(res))
			return res;
	}

	return Result(Success);
}

Result GrafImageVulkan::CreateVkImageViews()
{
	if (VK_NULL_HANDLE == this->vkImage || this->vkImageView != VK_NULL_HANDLE)
	{
		return ResultError(InvalidArgs, std::string("GrafImageVulkan: failed to create image views, invalid parameters"));
	}

	VkImageType vkImageType = GrafUtilsVulkan::GrafToVkImageType(this->GetDesc().Type);
	VkImageViewType vkViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
	switch (vkImageType)
	{
	case VK_IMAGE_TYPE_1D: vkViewType = VK_IMAGE_VIEW_TYPE_1D; break;
	case VK_IMAGE_TYPE_2D: vkViewType = VK_IMAGE_VIEW_TYPE_2D; break;
	case VK_IMAGE_TYPE_3D: vkViewType = VK_IMAGE_VIEW_TYPE_3D; break;
	// NOTE: handle texture arrays
	//VK_IMAGE_VIEW_TYPE_CUBE;
	//VK_IMAGE_VIEW_TYPE_1D_ARRAY;
	//VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	//VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
	default:
		return ResultError(InvalidArgs, "GrafImageVulkan: failed to create image views, unsupported image type");
	};

	VkImageViewCreateInfo vkViewInfo = {};
	vkViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	vkViewInfo.flags = 0;
	vkViewInfo.image = this->vkImage;
	vkViewInfo.viewType = vkViewType;
	vkViewInfo.format = GrafUtilsVulkan::GrafToVkFormat(this->GetDesc().Format);
	vkViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	vkViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	vkViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	vkViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	vkViewInfo.subresourceRange.aspectMask = GrafUtilsVulkan::GrafToVkImageUsageAspect(this->GetDesc().Usage);
	vkViewInfo.subresourceRange.baseMipLevel = 0;
	vkViewInfo.subresourceRange.levelCount = this->GetDesc().MipLevels;
	vkViewInfo.subresourceRange.baseArrayLayer = 0;
	vkViewInfo.subresourceRange.layerCount = 1;

	GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice()); // at this point device expected to be validate
	VkResult res = vkCreateImageView(grafDeviceVulkan->GetVkDevice(), &vkViewInfo, ur_null, &this->vkImageView);
	if (res != VK_SUCCESS)
		return ResultError(Failure, std::string("GrafImageVulkan: vkCreateImageView failed with VkResult = ") + VkResultToString(res));

	return Result(Success);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GrafBufferVulkan::GrafBufferVulkan(GrafSystem &grafSystem) :
	GrafBuffer(grafSystem)
{
	this->vkBuffer = VK_NULL_HANDLE;
	this->vkDeviceMemory = VK_NULL_HANDLE;
	this->vkDeviceMemoryOffset = 0;
	this->vkDeviceMemorySize = 0;
	this->vkDeviceMemoryAlignment = 0;
}

GrafBufferVulkan::~GrafBufferVulkan()
{
	this->Deinitialize();
}

Result GrafBufferVulkan::Deinitialize()
{
	this->vkDeviceMemoryOffset = 0;
	this->vkDeviceMemorySize = 0;
	this->vkDeviceMemoryAlignment = 0;
	if (this->vkDeviceMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkDeviceMemory, ur_null);
		this->vkDeviceMemory = VK_NULL_HANDLE;
	}
	if (this->vkBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkBuffer, ur_null);
		this->vkBuffer = VK_NULL_HANDLE;
	}

	return Result(Success);
}

Result GrafBufferVulkan::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
{
	this->Deinitialize();

	GrafBuffer::Initialize(grafDevice, initParams);

	// validate logical device

	GrafSystemVulkan& grafSystemVulkan = static_cast<GrafSystemVulkan&>(this->GetGrafSystem());
	GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
	if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
	{
		return ResultError(InvalidArgs, std::string("GrafBufferVulkan: failed to initialize, invalid GrafDevice"));
	}
	VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();
	VkPhysicalDevice vkPhysicalDevice = grafSystemVulkan.GetVkPhysicalDevice(grafDeviceVulkan->GetDeviceId());
	ur_uint32 vkDeviceGraphicsQueueId = (ur_uint32)grafDeviceVulkan->GetVkDeviceGraphicsQueueId();

	// create buffer

	VkBufferCreateInfo vkBufferInfo = {};
	vkBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vkBufferInfo.flags = 0;
	vkBufferInfo.size = (VkDeviceSize)initParams.BufferDesc.SizeInBytes;
	vkBufferInfo.usage = GrafUtilsVulkan::GrafToVkBufferUsage(initParams.BufferDesc.Usage);
	vkBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vkBufferInfo.queueFamilyIndexCount = 1;
	vkBufferInfo.pQueueFamilyIndices = &vkDeviceGraphicsQueueId;

	VkResult vkRes = vkCreateBuffer(vkDevice, &vkBufferInfo, ur_null, &this->vkBuffer);
	if (vkRes != VK_SUCCESS)
	{
		this->Deinitialize();
		return ResultError(Failure, std::string("GrafBufferVulkan: vkCreateBuffer failed with VkResult = ") + VkResultToString(vkRes));
	}

	// TODO: move allocation to GrafDevice (allocate big chunks of memory and return offsets, consider integrating VulkanAllocator)
	// request buffer memory requirements and find corresponding physical device memory type for allocation

	VkMemoryPropertyFlags vkMemoryPropertiesExpected = GrafUtilsVulkan::GrafToVkMemoryProperties(initParams.BufferDesc.MemoryType);

	VkMemoryRequirements vkMemoryRequirements = {};
	vkGetBufferMemoryRequirements(vkDevice, this->vkBuffer, &vkMemoryRequirements);

	VkPhysicalDeviceMemoryProperties vkDeviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkDeviceMemoryProperties);

	VkMemoryAllocateInfo vkAllocateInfo = {};
	vkAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	vkAllocateInfo.allocationSize = vkMemoryRequirements.size;
	vkAllocateInfo.memoryTypeIndex = ur_uint32(-1);
	for (ur_uint32 itype = 0; itype < vkDeviceMemoryProperties.memoryTypeCount; ++itype)
	{
		if (vkDeviceMemoryProperties.memoryTypes[itype].propertyFlags & vkMemoryPropertiesExpected)
		{
			vkAllocateInfo.memoryTypeIndex = itype;
			break;
		}
	}
	if (ur_uint32(-1) == vkAllocateInfo.memoryTypeIndex)
	{
		this->Deinitialize();
		return ResultError(Failure, "GrafBufferVulkan: failed to find required memory type");
	}

	// allocate memory

	vkRes = vkAllocateMemory(vkDevice, &vkAllocateInfo, ur_null, &this->vkDeviceMemory);
	if (vkRes != VK_SUCCESS)
	{
		this->Deinitialize();
		return ResultError(Failure, std::string("GrafBufferVulkan: vkAllocateMemory failed with VkResult = ") + VkResultToString(vkRes));
	}

	// bind device memory allocation to buffer

	vkRes = vkBindBufferMemory(vkDevice, this->vkBuffer, this->vkDeviceMemory, 0);
	if (vkRes != VK_SUCCESS)
	{
		this->Deinitialize();
		return ResultError(Failure, std::string("GrafBufferVulkan: vkBindBufferMemory failed with VkResult = ") + VkResultToString(vkRes));
	}

	this->vkDeviceMemoryOffset = 0;
	this->vkDeviceMemorySize = vkMemoryRequirements.size;
	this->vkDeviceMemoryAlignment = vkMemoryRequirements.alignment;

	return Result(Success);
}

Result GrafBufferVulkan::Write(ur_byte* dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
{
	if (0 == dataSize)
		dataSize = this->GetDesc().SizeInBytes; // entire allocation range

	if (ur_null == dataPtr || dstOffset + dataSize > this->vkDeviceMemorySize)
		return Result(InvalidArgs);

	GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice());
	VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

	void* mappedMemoryPtr = ur_null;
	VkResult vkRes = vkMapMemory(vkDevice, this->vkDeviceMemory, this->vkDeviceMemoryOffset + dstOffset, dataSize, 0, &mappedMemoryPtr);
	if (vkRes != VK_SUCCESS)
	{
		return ResultError(Failure, std::string("GrafBufferVulkan: vkMapMemory failed with VkResult = ") + VkResultToString(vkRes));
	}

	memcpy(mappedMemoryPtr, dataPtr + srcOffset, dataSize);

	vkUnmapMemory(vkDevice, this->vkDeviceMemory);

	return Result(Success);
}

Result GrafBufferVulkan::Read(ur_byte*& dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
{
	if (0 == dataSize)
		dataSize = this->GetDesc().SizeInBytes; // entire allocation range

	if (ur_null == dataPtr || srcOffset + dataSize > this->vkDeviceMemorySize)
		return Result(InvalidArgs);

	GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice());
	VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

	void* mappedMemoryPtr = ur_null;
	VkResult vkRes = vkMapMemory(vkDevice, this->vkDeviceMemory, this->vkDeviceMemoryOffset + srcOffset, dataSize, 0, &mappedMemoryPtr);
	if (vkRes != VK_SUCCESS)
	{
		return ResultError(Failure, std::string("GrafBufferVulkan: vkMapMemory failed with VkResult = ") + VkResultToString(vkRes));
	}

	memcpy(dataPtr + dstOffset, mappedMemoryPtr, dataSize);

	vkUnmapMemory(vkDevice, this->vkDeviceMemory);

	return Result(Success);
}

Result GrafBufferVulkan::Transfer(GrafBuffer* dstBuffer, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
{
	return Result(NotImplemented);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GrafShaderVulkan::GrafShaderVulkan(GrafSystem &grafSystem) : 
	GrafShader(grafSystem)
{
	this->vkShaderModule = VK_NULL_HANDLE;
}

GrafShaderVulkan::~GrafShaderVulkan()
{
	this->Deinitialize();
}

Result GrafShaderVulkan::Deinitialize()
{
	if (this->vkShaderModule != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkShaderModule, ur_null);
		this->vkShaderModule = VK_NULL_HANDLE;
	}

	return Result(Success);
}

Result GrafShaderVulkan::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
{
	this->Deinitialize();

	GrafShader::Initialize(grafDevice, initParams);

	// validate logical device 

	GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
	if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
	{
		return ResultError(InvalidArgs, std::string("GrafShaderVulkan: failed to initialize, invalid GrafDevice"));
	}
	VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

	// create shader module

	VkShaderModuleCreateInfo vkShaderInfo = {};
	vkShaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vkShaderInfo.flags = 0;
	vkShaderInfo.codeSize = initParams.ByteCodeSize;
	vkShaderInfo.pCode = (const uint32_t*)initParams.ByteCode;

	VkResult vkRes = vkCreateShaderModule(vkDevice, &vkShaderInfo, ur_null, &this->vkShaderModule);
	if (vkRes != VK_SUCCESS)
		return ResultError(Failure, std::string("GrafShaderVulkan: vkCreateShaderModule failed with VkResult = ") + VkResultToString(vkRes));

	return Result(Success);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GrafRenderPassVulkan::GrafRenderPassVulkan(GrafSystem& grafSystem) :
	GrafRenderPass(grafSystem)
{
	this->vkRenderPass = VK_NULL_HANDLE;
}

GrafRenderPassVulkan::~GrafRenderPassVulkan()
{
	this->Deinitialize();
}

Result GrafRenderPassVulkan::Deinitialize()
{
	if (this->vkRenderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkRenderPass, ur_null);
		this->vkRenderPass = VK_NULL_HANDLE;
	}

	return Result(Success);
}

Result GrafRenderPassVulkan::Initialize(GrafDevice* grafDevice)
{
	this->Deinitialize();

	GrafRenderPass::Initialize(grafDevice);

	// validate logical device 

	GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
	if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
	{
		return ResultError(InvalidArgs, std::string("GrafImageVulkan: failed to initialize, invalid GrafDevice"));
	}
	VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

	// TEMP: hardcoded test pass configuration

	VkAttachmentDescription vkAttachmentDesc = {};
	vkAttachmentDesc.flags = 0;
	vkAttachmentDesc.format = VK_FORMAT_B8G8R8A8_UNORM;
	vkAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	vkAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	vkAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	vkAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	vkAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	vkAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_GENERAL;// GrafUtilsVulkan::GrafToVkImageLayout(image->GetState());
	vkAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_GENERAL;// (VK_IMAGE_LAYOUT_UNDEFINED == vkAttachmentDesc.initialLayout ? VK_IMAGE_LAYOUT_GENERAL : vkAttachmentDesc.initialLayout);

	VkAttachmentReference vkColorAttachmentRef = {};
	vkColorAttachmentRef.attachment = 0;
	vkColorAttachmentRef.layout = VK_IMAGE_LAYOUT_GENERAL;

	VkSubpassDescription vkSubpassDesc = {};
	vkSubpassDesc.flags = 0;
	vkSubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	vkSubpassDesc.inputAttachmentCount = 0;
	vkSubpassDesc.pInputAttachments = ur_null;
	vkSubpassDesc.colorAttachmentCount = 1;
	vkSubpassDesc.pColorAttachments = &vkColorAttachmentRef;
	vkSubpassDesc.pResolveAttachments = ur_null;
	vkSubpassDesc.pDepthStencilAttachment = ur_null;
	vkSubpassDesc.preserveAttachmentCount = 0;
	vkSubpassDesc.pPreserveAttachments = ur_null;

	VkRenderPassCreateInfo vkRenderPassInfo = {};
	vkRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	vkRenderPassInfo.flags = 0;
	vkRenderPassInfo.attachmentCount = 1;
	vkRenderPassInfo.pAttachments = &vkAttachmentDesc;
	vkRenderPassInfo.subpassCount = 1;
	vkRenderPassInfo.pSubpasses = &vkSubpassDesc;
	vkRenderPassInfo.dependencyCount = 0;
	vkRenderPassInfo.pDependencies = ur_null;

	VkResult vkRes = vkCreateRenderPass(vkDevice, &vkRenderPassInfo, ur_null, &this->vkRenderPass);
	if (vkRes != VK_SUCCESS)
	{
		this->Deinitialize();
		return ResultError(Failure, std::string("GrafRenderPassVulkan: vkCreateRenderPass failed with VkResult = ") + VkResultToString(vkRes));
	}

	return Result(Success);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GrafRenderTargetVulkan::GrafRenderTargetVulkan(GrafSystem &grafSystem) :
	GrafRenderTarget(grafSystem)
{
	this->vkFramebuffer = VK_NULL_HANDLE;
}

GrafRenderTargetVulkan::~GrafRenderTargetVulkan()
{
	this->Deinitialize();
}

Result GrafRenderTargetVulkan::Deinitialize()
{
	if (this->vkFramebuffer != VK_NULL_HANDLE)
	{
		vkDestroyFramebuffer(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkFramebuffer, ur_null);
		this->vkFramebuffer = VK_NULL_HANDLE;
	}

	this->images.clear();
	this->renderPass = ur_null;

	return Result(Success);
}

Result GrafRenderTargetVulkan::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
{
	this->Deinitialize();

	if (ur_null == initParams.RenderPass || initParams.Images.empty())
		return Result(InvalidArgs);

	GrafRenderTarget::Initialize(grafDevice, initParams);

	// validate logical device

	GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
	if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
	{
		return ResultError(InvalidArgs, std::string("GrafImageVulkan: failed to initialize, invalid GrafDevice"));
	}
	VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();

	// create frame buffer object

	std::vector<VkImageView> attachmentViews(initParams.Images.size());
	for (ur_size iimage = 0; iimage < initParams.Images.size(); ++iimage)
	{
		attachmentViews[iimage] = static_cast<GrafImageVulkan*>(initParams.Images[iimage])->GetVkImageView();
	}

	VkFramebufferCreateInfo vkFramebufferInfo = {};
	vkFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	vkFramebufferInfo.flags = 0;
	vkFramebufferInfo.renderPass = static_cast<GrafRenderPassVulkan*>(initParams.RenderPass)->GetVkRenderPass();
	vkFramebufferInfo.attachmentCount = (ur_uint32)attachmentViews.size();
	vkFramebufferInfo.pAttachments = attachmentViews.data();
	vkFramebufferInfo.width = (ur_uint32)initParams.Images[0]->GetDesc().Size.x;
	vkFramebufferInfo.height = (ur_uint32)initParams.Images[0]->GetDesc().Size.y;
	vkFramebufferInfo.layers = 1;

	VkResult vkRes = vkCreateFramebuffer(vkDevice, &vkFramebufferInfo, ur_null, &this->vkFramebuffer);
	if (vkRes != VK_SUCCESS)
	{
		this->Deinitialize();
		return ResultError(Failure, std::string("GrafRenderTargetVulkan: vkCreateFramebuffer failed with VkResult = ") + VkResultToString(vkRes));
	}

	return Result(Success);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GrafPipelineVulkan::GrafPipelineVulkan(GrafSystem &grafSystem) :
	GrafPipeline(grafSystem)
{
	this->vkPipeline = VK_NULL_HANDLE;
	this->vkPipelineLayout = VK_NULL_HANDLE;
	this->vkDescriptorSetLayout = VK_NULL_HANDLE;
	this->vkDescriptorPool = VK_NULL_HANDLE;
}

GrafPipelineVulkan::~GrafPipelineVulkan()
{
	this->Deinitialize();
}

Result GrafPipelineVulkan::Deinitialize()
{
	if (!this->vkDescriptorSets.empty())
	{
		vkFreeDescriptorSets(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkDescriptorPool, 1, this->vkDescriptorSets.data());
		this->vkDescriptorSets.clear();
	}
	if (this->vkPipelineLayout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkPipelineLayout, ur_null);
		this->vkPipelineLayout = VK_NULL_HANDLE;
	}
	if (this->vkDescriptorSetLayout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkDescriptorSetLayout, ur_null);
		this->vkDescriptorSetLayout = VK_NULL_HANDLE;
	}
	if (this->vkDescriptorPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkDescriptorPool, ur_null);
		this->vkDescriptorPool = VK_NULL_HANDLE;
	}
	if (this->vkPipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice(), this->vkPipeline, ur_null);
		this->vkPipeline = VK_NULL_HANDLE;
	}

	return Result(Success);
}

Result GrafPipelineVulkan::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
{
	this->Deinitialize();

	if (ur_null == initParams.RenderPass)
		return Result(InvalidArgs);

	GrafPipeline::Initialize(grafDevice, initParams);

	// validate logical device 

	GrafDeviceVulkan* grafDeviceVulkan = static_cast<GrafDeviceVulkan*>(grafDevice);
	if (ur_null == grafDeviceVulkan || VK_NULL_HANDLE == grafDeviceVulkan->GetVkDevice())
	{
		return ResultError(InvalidArgs, std::string("GrafPipelineVulkan: failed to initialize, invalid GrafDevice"));
	}
	VkDevice vkDevice = grafDeviceVulkan->GetVkDevice();
	VkRenderPass vkRenderPass = static_cast<GrafRenderPassVulkan*>(initParams.RenderPass)->GetVkRenderPass();
	VkResult vkRes = VK_SUCCESS;

	// TEMP: hardcoded pipeline state for test purpose
	// TODO: must be described through GRAF structures and converted into vulkan here

	// shader stages

	std::vector<VkPipelineShaderStageCreateInfo> vkShaderStagesInfo(initParams.ShaderStageCount);
	for (ur_uint istage = 0; istage < initParams.ShaderStageCount; ++istage)
	{
		GrafShader* grafShader = initParams.ShaderStages[istage];
		VkPipelineShaderStageCreateInfo& vkShaderStageInfo = vkShaderStagesInfo[istage];
		vkShaderStageInfo = {};
		vkShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vkShaderStageInfo.flags = 0;
		vkShaderStageInfo.stage = GrafUtilsVulkan::GrafToVkShaderStage(grafShader->GetShaderType());
		vkShaderStageInfo.module = static_cast<GrafShaderVulkan*>(grafShader)->GetVkShaderModule();
		vkShaderStageInfo.pName = grafShader->GetEntryPoint().c_str();
	}

	// pipeline layout

	// TEMP: hardcoded sample layout
	VkDescriptorSetLayoutBinding vkDescriptorSetBindingInfo = {};
	vkDescriptorSetBindingInfo.binding = 0;
	vkDescriptorSetBindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vkDescriptorSetBindingInfo.descriptorCount = 1;
	vkDescriptorSetBindingInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	vkDescriptorSetBindingInfo.pImmutableSamplers = ur_null;
	
	VkDescriptorSetLayoutCreateInfo vkDescriptorSetInfo = {};
	vkDescriptorSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	vkDescriptorSetInfo.bindingCount = 1;
	vkDescriptorSetInfo.pBindings = &vkDescriptorSetBindingInfo;
	vkRes = vkCreateDescriptorSetLayout(vkDevice, &vkDescriptorSetInfo, ur_null, &this->vkDescriptorSetLayout);
	if (vkRes != VK_SUCCESS)
	{
		this->Deinitialize();
		return ResultError(Failure, std::string("GrafPipelineVulkan: vkCreateDescriptorSetLayout failed with VkResult = ") + VkResultToString(vkRes));
	}

	VkDescriptorPoolSize vkDescriptorPoolSize = {};
	vkDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vkDescriptorPoolSize.descriptorCount = 3;

	VkDescriptorPoolCreateInfo vkDescriptorPoolInfo = {};
	vkDescriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	vkDescriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	vkDescriptorPoolInfo.maxSets = 3;
	vkDescriptorPoolInfo.poolSizeCount = 1;
	vkDescriptorPoolInfo.pPoolSizes = &vkDescriptorPoolSize;
	vkRes = vkCreateDescriptorPool(vkDevice, &vkDescriptorPoolInfo, ur_null, &this->vkDescriptorPool);
	if (vkRes != VK_SUCCESS)
	{
		this->Deinitialize();
		return ResultError(Failure, std::string("GrafPipelineVulkan: vkCreateDescriptorPool failed with VkResult = ") + VkResultToString(vkRes));
	}

	VkDescriptorSetAllocateInfo vkDescriptorSetAllocateInfo = {};
	vkDescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	vkDescriptorSetAllocateInfo.descriptorPool = this->vkDescriptorPool;
	vkDescriptorSetAllocateInfo.descriptorSetCount = 3;
	std::vector<VkDescriptorSetLayout> vkDescritorSetLayouts(3, this->vkDescriptorSetLayout);
	vkDescriptorSetAllocateInfo.pSetLayouts = vkDescritorSetLayouts.data();
	this->vkDescriptorSets.resize(vkDescriptorSetAllocateInfo.descriptorSetCount);
	vkRes = vkAllocateDescriptorSets(vkDevice, &vkDescriptorSetAllocateInfo, this->vkDescriptorSets.data());
	if (vkRes != VK_SUCCESS)
	{
		this->Deinitialize();
		return ResultError(Failure, std::string("vkAllocateDescriptorSets: vkCreateDescriptorPool failed with VkResult = ") + VkResultToString(vkRes));
	}

	VkPipelineLayoutCreateInfo vkPipelineLayoutInfo = {};
	vkPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	vkPipelineLayoutInfo.flags = 0;
	vkPipelineLayoutInfo.setLayoutCount = 1;
	vkPipelineLayoutInfo.pSetLayouts = &this->vkDescriptorSetLayout;
	vkPipelineLayoutInfo.pushConstantRangeCount = 0;
	vkPipelineLayoutInfo.pPushConstantRanges = ur_null;

	vkRes = vkCreatePipelineLayout(vkDevice, &vkPipelineLayoutInfo, ur_null, &this->vkPipelineLayout);
	if (vkRes != VK_SUCCESS)
	{
		this->Deinitialize();
		return ResultError(Failure, std::string("GrafPipelineVulkan: vkCreatePipelineLayout failed with VkResult = ") + VkResultToString(vkRes));
	}

	// vertex input

	ur_uint vertexInputTotalElementCount = 0;
	for (ur_uint ivb = 0; ivb < initParams.VertexInputCount; ++ivb)
	{
		vertexInputTotalElementCount += initParams.VertexInputDesc[ivb].ElementCount;
	}

	std::vector<VkVertexInputAttributeDescription> vkVertexAttributes(vertexInputTotalElementCount);
	std::vector<VkVertexInputBindingDescription> vkVertexBindingDescs(initParams.VertexInputCount);
	ur_uint vertexAttributesOffset = 0;
	for (ur_uint ivb = 0; ivb < initParams.VertexInputCount; ++ivb)
	{
		const GrafVertexInputDesc& grafVertexInputDesc = initParams.VertexInputDesc[ivb];
		VkVertexInputBindingDescription& vkVertexBindingDesc = vkVertexBindingDescs[ivb];
		vkVertexBindingDesc.binding = (ur_uint32)grafVertexInputDesc.BindingIdx;
		vkVertexBindingDesc.stride = (ur_uint32)grafVertexInputDesc.Stride;
		vkVertexBindingDesc.inputRate = (GrafVertexInputType::PerInstance == grafVertexInputDesc.InputType ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX);

		for (ur_uint ielem = 0; ielem < grafVertexInputDesc.ElementCount; ++ielem)
		{
			const GrafVertexElementDesc& grafVertexElement = grafVertexInputDesc.Elements[ielem];
			VkVertexInputAttributeDescription& vkVertexAttribute = vkVertexAttributes[vertexAttributesOffset + ielem];
			vkVertexAttribute.location = (ur_uint32)ielem;
			vkVertexAttribute.binding = (ur_uint32)ivb;
			vkVertexAttribute.format = GrafUtilsVulkan::GrafToVkFormat(grafVertexElement.Format);
			vkVertexAttribute.offset = (ur_uint32)grafVertexElement.Offset;
		}
		vertexAttributesOffset += grafVertexInputDesc.ElementCount;
	}

	VkPipelineVertexInputStateCreateInfo vkVertxInputStateInfo = {};
	vkVertxInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vkVertxInputStateInfo.flags = 0;
	vkVertxInputStateInfo.vertexBindingDescriptionCount = (ur_uint32)vkVertexBindingDescs.size();
	vkVertxInputStateInfo.pVertexBindingDescriptions = vkVertexBindingDescs.data();
	vkVertxInputStateInfo.vertexAttributeDescriptionCount = (ur_uint32)vkVertexAttributes.size();
	vkVertxInputStateInfo.pVertexAttributeDescriptions = vkVertexAttributes.data();

	// input assembly

	VkPipelineInputAssemblyStateCreateInfo vkInputAssemblyInfo = {};
	vkInputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	vkInputAssemblyInfo.flags = 0;
	vkInputAssemblyInfo.topology = GrafUtilsVulkan::GrafToVkPrimitiveTopology(initParams.PrimitiveTopology);
	vkInputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	//  tessellation state

	// VkPipelineTessellationStateCreateInfo;

	// viewport & scissors

	VkViewport vkViewport = {};
	vkViewport.x = initParams.ViewportDesc.X;
	vkViewport.y = initParams.ViewportDesc.Y;
	vkViewport.width = initParams.ViewportDesc.Width;
	vkViewport.height = initParams.ViewportDesc.Height;
	vkViewport.minDepth = initParams.ViewportDesc.Near;
	vkViewport.maxDepth = initParams.ViewportDesc.Far;

	VkRect2D vkScissorRect = { { (int32_t)vkViewport.x, (int32_t)vkViewport.y}, { (uint32_t)vkViewport.width, (uint32_t)vkViewport.height} };

	VkPipelineViewportStateCreateInfo vkViewportStateInfo = {};
	vkViewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vkViewportStateInfo.flags = 0;
	vkViewportStateInfo.viewportCount = 1;
	vkViewportStateInfo.pViewports = &vkViewport;
	vkViewportStateInfo.scissorCount = 1;
	vkViewportStateInfo.pScissors = &vkScissorRect;

	// rasterizer state

	VkPipelineRasterizationStateCreateInfo vkRasterStateInfo = {};
	vkRasterStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	vkRasterStateInfo.flags = 0;
	vkRasterStateInfo.depthClampEnable = VK_FALSE;
	vkRasterStateInfo.rasterizerDiscardEnable = VK_FALSE;
	vkRasterStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	vkRasterStateInfo.cullMode = VK_CULL_MODE_NONE;
	vkRasterStateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	vkRasterStateInfo.depthBiasEnable = VK_FALSE;
	vkRasterStateInfo.depthBiasConstantFactor = 0.0f;
	vkRasterStateInfo.depthBiasClamp = 0.0f;
	vkRasterStateInfo.depthBiasSlopeFactor = 0.0f;
	vkRasterStateInfo.lineWidth = 1.0f;

	// multisampling

	VkPipelineMultisampleStateCreateInfo vkMultisampleStateInfo = {};
	vkMultisampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	vkMultisampleStateInfo.flags = 0;
	vkMultisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	vkMultisampleStateInfo.sampleShadingEnable = VK_FALSE;
	vkMultisampleStateInfo.minSampleShading = 1.0f;
	vkMultisampleStateInfo.pSampleMask = ur_null;
	vkMultisampleStateInfo.alphaToCoverageEnable = VK_FALSE;
	vkMultisampleStateInfo.alphaToOneEnable = VK_FALSE;

	// depth stencil state

	VkPipelineDepthStencilStateCreateInfo vkDepthStencilStateInfo = {};
	vkDepthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	vkDepthStencilStateInfo.flags = 0;
	vkDepthStencilStateInfo.depthTestEnable = VK_FALSE;
	vkDepthStencilStateInfo.depthWriteEnable = VK_FALSE;
	vkDepthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_NEVER;
	vkDepthStencilStateInfo.depthBoundsTestEnable = VK_FALSE;
	vkDepthStencilStateInfo.stencilTestEnable = VK_FALSE;
	vkDepthStencilStateInfo.front = {};
	vkDepthStencilStateInfo.back = {};
	vkDepthStencilStateInfo.minDepthBounds = 0.0f;
	vkDepthStencilStateInfo.maxDepthBounds = 0.0f;

	// color blend state

	VkPipelineColorBlendAttachmentState vkAttachmentBlendState = {};
	vkAttachmentBlendState.blendEnable = VK_FALSE;
	vkAttachmentBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	vkAttachmentBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	vkAttachmentBlendState.colorBlendOp = VK_BLEND_OP_ADD;
	vkAttachmentBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	vkAttachmentBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	vkAttachmentBlendState.alphaBlendOp = VK_BLEND_OP_ADD;
	vkAttachmentBlendState.colorWriteMask = (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

	VkPipelineColorBlendStateCreateInfo vkColorBlendStateInfo = {};
	vkColorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	vkColorBlendStateInfo.flags = 0;
	vkColorBlendStateInfo.logicOpEnable = VK_FALSE;
	vkColorBlendStateInfo.logicOp = VK_LOGIC_OP_COPY;
	vkColorBlendStateInfo.attachmentCount = 1;
	vkColorBlendStateInfo.pAttachments = &vkAttachmentBlendState;
	vkColorBlendStateInfo.blendConstants[0] = 0.0f;
	vkColorBlendStateInfo.blendConstants[1] = 0.0f;
	vkColorBlendStateInfo.blendConstants[2] = 0.0f;
	vkColorBlendStateInfo.blendConstants[3] = 0.0f;

	// dynamic state

	VkDynamicState vkDynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_LINE_WIDTH,
		VK_DYNAMIC_STATE_DEPTH_BIAS,
		VK_DYNAMIC_STATE_BLEND_CONSTANTS,
		VK_DYNAMIC_STATE_DEPTH_BOUNDS,
		VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
		VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
		VK_DYNAMIC_STATE_STENCIL_REFERENCE
	};

	VkPipelineDynamicStateCreateInfo vkDynamicStateInfo = {};
	vkDynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	vkDynamicStateInfo.flags = 0;
	vkDynamicStateInfo.dynamicStateCount = ur_array_size(vkDynamicStates);
	vkDynamicStateInfo.pDynamicStates = vkDynamicStates;

	// create pipeline object

	VkGraphicsPipelineCreateInfo vkPipelineInfo = {};
	vkPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	vkPipelineInfo.flags = 0; // NOTE: consider VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR for debug purpose
	vkPipelineInfo.stageCount = (ur_uint32)vkShaderStagesInfo.size();
	vkPipelineInfo.pStages = vkShaderStagesInfo.data();
	vkPipelineInfo.pVertexInputState = &vkVertxInputStateInfo;
	vkPipelineInfo.pInputAssemblyState = &vkInputAssemblyInfo;
	vkPipelineInfo.pTessellationState = ur_null;
	vkPipelineInfo.pViewportState = &vkViewportStateInfo;
	vkPipelineInfo.pRasterizationState = &vkRasterStateInfo;
	vkPipelineInfo.pMultisampleState = &vkMultisampleStateInfo;
	vkPipelineInfo.pDepthStencilState = &vkDepthStencilStateInfo;
	vkPipelineInfo.pColorBlendState = &vkColorBlendStateInfo;
	vkPipelineInfo.pDynamicState = &vkDynamicStateInfo;
	vkPipelineInfo.layout = this->vkPipelineLayout;
	vkPipelineInfo.renderPass = vkRenderPass;
	vkPipelineInfo.subpass = 0;
	vkPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	vkPipelineInfo.basePipelineIndex = -1;

	vkRes = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &vkPipelineInfo, ur_null, &this->vkPipeline);
	if (vkRes != VK_SUCCESS)
	{
		this->Deinitialize();
		return ResultError(Failure, std::string("GrafPipelineVulkan: vkCreateGraphicsPipelines failed with VkResult = ") + VkResultToString(vkRes));
	}

	return Result(Success);
}

Result GrafPipelineVulkan::UpdateConstantBuffer(ur_uint setIdx, GrafBuffer* buffer)
{
	VkDevice vkDevice = static_cast<GrafDeviceVulkan*>(this->GetGrafDevice())->GetVkDevice();
	setIdx = (setIdx % this->vkDescriptorSets.size());

	VkDescriptorBufferInfo vkDescriptorBufferInfo = {};
	vkDescriptorBufferInfo.buffer = static_cast<GrafBufferVulkan*>(buffer)->GetVkBuffer();
	vkDescriptorBufferInfo.offset = 0;
	vkDescriptorBufferInfo.range = (VkDeviceSize)buffer->GetDesc().SizeInBytes;

	VkWriteDescriptorSet vkWriteDescriptorSet = {};
	vkWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	vkWriteDescriptorSet.dstSet = this->vkDescriptorSets[setIdx];
	vkWriteDescriptorSet.dstBinding = 0;
	vkWriteDescriptorSet.dstArrayElement = 0;
	vkWriteDescriptorSet.descriptorCount = 1;
	vkWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vkWriteDescriptorSet.pImageInfo = ur_null;
	vkWriteDescriptorSet.pBufferInfo = &vkDescriptorBufferInfo;
	vkWriteDescriptorSet.pTexelBufferView = ur_null;

	vkUpdateDescriptorSets(vkDevice, 1, &vkWriteDescriptorSet, 0, ur_null);

	return Result(Success);
}

Result GrafPipelineVulkan::BindDescriptorSet(ur_uint setIdx, GrafCommandList* commandList)
{
	setIdx = (setIdx % this->vkDescriptorSets.size());

	VkCommandBuffer vkCommandBuffer = static_cast<GrafCommandListVulkan*>(commandList)->GetVkCommandBuffer();
	vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->vkPipelineLayout, 0, 1, &this->vkDescriptorSets[setIdx], 0, ur_null);

	return Result(Success);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VkImageUsageFlags GrafUtilsVulkan::GrafToVkImageUsage(GrafImageUsageFlags usage)
{
	VkImageUsageFlags vkImageUsage = 0;
	if (usage & (ur_uint)GrafImageUsageFlag::ColorRenderTarget)
		vkImageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (usage & (ur_uint)GrafImageUsageFlag::DepthStencilRenderTarget)
		vkImageUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if (usage & (ur_uint)GrafImageUsageFlag::TransferSrc)
		vkImageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	if (usage & (ur_uint)GrafImageUsageFlag::TransferDst)
		vkImageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	return vkImageUsage;
}

GrafImageUsageFlags GrafUtilsVulkan::VkToGrafImageUsage(VkImageUsageFlags usage)
{
	GrafImageUsageFlags grafUsage = (ur_uint)GrafImageUsageFlag::Undefined;
	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		grafUsage |= (ur_uint)GrafImageUsageFlag::ColorRenderTarget;
	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		grafUsage |= (ur_uint)GrafImageUsageFlag::DepthStencilRenderTarget;
	if (VK_IMAGE_USAGE_TRANSFER_SRC_BIT & usage)
		grafUsage |= (ur_uint)GrafImageUsageFlag::TransferSrc;
	if (VK_IMAGE_USAGE_TRANSFER_DST_BIT & usage)
		grafUsage |= (ur_uint)GrafImageUsageFlag::TransferDst;
	return grafUsage;
}

VkImageType GrafUtilsVulkan::GrafToVkImageType(GrafImageType imageType)
{
	VkImageType vkImageType = VK_IMAGE_TYPE_MAX_ENUM;
	switch (imageType)
	{
	case GrafImageType::Tex1D: vkImageType = VK_IMAGE_TYPE_1D; break;
	case GrafImageType::Tex2D: vkImageType = VK_IMAGE_TYPE_2D; break;
	case GrafImageType::Tex3D: vkImageType = VK_IMAGE_TYPE_3D; break;
	};
	return vkImageType;
}

VkImageAspectFlags GrafUtilsVulkan::GrafToVkImageUsageAspect(GrafImageUsageFlags usage)
{
	VkImageAspectFlags vkImageAspectFlags = 0;
	if (usage & (ur_uint)GrafImageUsageFlag::ColorRenderTarget)
		vkImageAspectFlags |= VK_IMAGE_ASPECT_COLOR_BIT;
	if (usage & (ur_uint)GrafImageUsageFlag::DepthStencilRenderTarget)
		vkImageAspectFlags |= (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
	return vkImageAspectFlags;
}

VkImageLayout GrafUtilsVulkan::GrafToVkImageLayout(GrafImageState imageState)
{
	VkImageLayout vkImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	switch (imageState)
	{
	case GrafImageState::Common: vkImageLayout = VK_IMAGE_LAYOUT_GENERAL; break;
	case GrafImageState::ColorWrite: vkImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; break;
	case GrafImageState::DepthStencilWrite: vkImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; break;
	case GrafImageState::DepthStencilRead: vkImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; break;
	case GrafImageState::ShaderRead: vkImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
	case GrafImageState::TransferSrc: vkImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; break;
	case GrafImageState::TransferDst: vkImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; break;
	case GrafImageState::Present: vkImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; break;
	};
	return vkImageLayout;
}

VkBufferUsageFlags GrafUtilsVulkan::GrafToVkBufferUsage(GrafBufferUsageFlags usage)
{
	VkBufferUsageFlags vkUsage = 0;
	if (usage & (ur_uint)GrafBufferUsageFlag::VertexBuffer)
		vkUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	if (usage & (ur_uint)GrafBufferUsageFlag::IndexBuffer)
		vkUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if (usage & (ur_uint)GrafBufferUsageFlag::ConstantBuffer)
		vkUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	return vkUsage;
}

VkMemoryPropertyFlags GrafUtilsVulkan::GrafToVkMemoryProperties(GrafDeviceMemoryFlags memoryType)
{
	VkMemoryPropertyFlags vkMemoryProperties = 0;
	if (memoryType & (ur_uint)GrafDeviceMemoryFlag::GpuLocal)
		vkMemoryProperties |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	if (memoryType & (ur_uint)GrafDeviceMemoryFlag::CpuVisible)
		vkMemoryProperties |= (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	return vkMemoryProperties;
}

VkShaderStageFlagBits GrafUtilsVulkan::GrafToVkShaderStage(GrafShaderType shaderType)
{
	VkShaderStageFlagBits vkShaderStage = VkShaderStageFlagBits(0);
	switch (shaderType)
	{
	case GrafShaderType::Vertex: vkShaderStage = VK_SHADER_STAGE_VERTEX_BIT;  break;
	case GrafShaderType::Pixel: vkShaderStage = VK_SHADER_STAGE_FRAGMENT_BIT;  break;
	case GrafShaderType::Compute: vkShaderStage = VK_SHADER_STAGE_COMPUTE_BIT;  break;
	};
	return vkShaderStage;
}

VkPrimitiveTopology GrafUtilsVulkan::GrafToVkPrimitiveTopology(GrafPrimitiveTopology topology)
{
	VkPrimitiveTopology vkTopology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	switch (topology)
	{
	case GrafPrimitiveTopology::PointList: vkTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; break;
	case GrafPrimitiveTopology::LineList: vkTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; break;
	case GrafPrimitiveTopology::LineStrip: vkTopology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP; break;
	case GrafPrimitiveTopology::TriangleList: vkTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;
	case GrafPrimitiveTopology::TriangleStrip: vkTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; break;
	case GrafPrimitiveTopology::TriangleFan: vkTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN; break;
	};
	return vkTopology;
}

static const VkFormat GrafToVkFormatLUT[ur_uint(GrafFormat::Count)] = {
	VK_FORMAT_UNDEFINED,
	VK_FORMAT_R8G8B8_UNORM,
	VK_FORMAT_R8G8B8_SNORM,
	VK_FORMAT_R8G8B8_UINT,
	VK_FORMAT_R8G8B8_SINT,
	VK_FORMAT_R8G8B8_SRGB,
	VK_FORMAT_B8G8R8_UNORM,
	VK_FORMAT_B8G8R8_SNORM,
	VK_FORMAT_B8G8R8_UINT,
	VK_FORMAT_B8G8R8_SINT,
	VK_FORMAT_B8G8R8_SRGB,
	VK_FORMAT_R8G8B8A8_UNORM,
	VK_FORMAT_R8G8B8A8_SNORM,
	VK_FORMAT_R8G8B8A8_UINT,
	VK_FORMAT_R8G8B8A8_SINT,
	VK_FORMAT_R8G8B8A8_SRGB,
	VK_FORMAT_B8G8R8A8_UNORM,
	VK_FORMAT_B8G8R8A8_SNORM,
	VK_FORMAT_B8G8R8A8_UINT,
	VK_FORMAT_B8G8R8A8_SINT,
	VK_FORMAT_B8G8R8A8_SRGB,
	VK_FORMAT_R32_SFLOAT,
	VK_FORMAT_R32G32_SFLOAT,
	VK_FORMAT_R32G32B32_SFLOAT,
	VK_FORMAT_R32G32B32A32_SFLOAT
};
VkFormat GrafUtilsVulkan::GrafToVkFormat(GrafFormat grafFormat)
{
	return GrafToVkFormatLUT[ur_uint(grafFormat)];
}

static const GrafFormat VkToGrafFormatLUT[VK_FORMAT_END_RANGE + 1] = {
	GrafFormat::Undefined,
	GrafFormat::Unsupported,			// VK_FORMAT_R4G4_UNORM_PACK8 = 1,
	GrafFormat::Unsupported,			// VK_FORMAT_R4G4B4A4_UNORM_PACK16 = 2,
	GrafFormat::Unsupported,			// VK_FORMAT_B4G4R4A4_UNORM_PACK16 = 3,
	GrafFormat::Unsupported,			// VK_FORMAT_R5G6B5_UNORM_PACK16 = 4,
	GrafFormat::Unsupported,			// VK_FORMAT_B5G6R5_UNORM_PACK16 = 5,
	GrafFormat::Unsupported,			// VK_FORMAT_R5G5B5A1_UNORM_PACK16 = 6,
	GrafFormat::Unsupported,			// VK_FORMAT_B5G5R5A1_UNORM_PACK16 = 7,
	GrafFormat::Unsupported,			// VK_FORMAT_A1R5G5B5_UNORM_PACK16 = 8,
	GrafFormat::Unsupported,			// VK_FORMAT_R8_UNORM = 9,
	GrafFormat::Unsupported,			// VK_FORMAT_R8_SNORM = 10,
	GrafFormat::Unsupported,			// VK_FORMAT_R8_USCALED = 11,
	GrafFormat::Unsupported,			// VK_FORMAT_R8_SSCALED = 12,
	GrafFormat::Unsupported,			// VK_FORMAT_R8_UINT = 13,
	GrafFormat::Unsupported,			// VK_FORMAT_R8_SINT = 14,
	GrafFormat::Unsupported,			// VK_FORMAT_R8_SRGB = 15,
	GrafFormat::Unsupported,			// VK_FORMAT_R8G8_UNORM = 16,
	GrafFormat::Unsupported,			// VK_FORMAT_R8G8_SNORM = 17,
	GrafFormat::Unsupported,			// VK_FORMAT_R8G8_USCALED = 18,
	GrafFormat::Unsupported,			// VK_FORMAT_R8G8_SSCALED = 19,
	GrafFormat::Unsupported,			// VK_FORMAT_R8G8_UINT = 20,
	GrafFormat::Unsupported,			// VK_FORMAT_R8G8_SINT = 21,
	GrafFormat::Unsupported,			// VK_FORMAT_R8G8_SRGB = 22,
	GrafFormat::R8G8B8_UNORM,			// VK_FORMAT_R8G8B8_UNORM = 23,
	GrafFormat::R8G8B8_SNORM,			// VK_FORMAT_R8G8B8_SNORM = 24,
	GrafFormat::Unsupported,			// VK_FORMAT_R8G8B8_USCALED = 25,
	GrafFormat::Unsupported,			// VK_FORMAT_R8G8B8_SSCALED = 26,
	GrafFormat::R8G8B8_UINT,			// VK_FORMAT_R8G8B8_UINT = 27,
	GrafFormat::R8G8B8_SINT,			// VK_FORMAT_R8G8B8_SINT = 28,
	GrafFormat::R8G8B8_SRGB,			// VK_FORMAT_R8G8B8_SRGB = 29,
	GrafFormat::B8G8R8_UNORM,			// VK_FORMAT_B8G8R8_UNORM = 30,
	GrafFormat::B8G8R8_SNORM,			// VK_FORMAT_B8G8R8_SNORM = 31,
	GrafFormat::Unsupported,			// VK_FORMAT_B8G8R8_USCALED = 32,
	GrafFormat::Unsupported,			// VK_FORMAT_B8G8R8_SSCALED = 33,
	GrafFormat::B8G8R8_UINT,			// VK_FORMAT_B8G8R8_UINT = 34,
	GrafFormat::B8G8R8_SINT,			// VK_FORMAT_B8G8R8_SINT = 35,
	GrafFormat::B8G8R8_SRGB,			// VK_FORMAT_B8G8R8_SRGB = 36,
	GrafFormat::R8G8B8A8_UNORM,			// VK_FORMAT_R8G8B8A8_UNORM = 37,
	GrafFormat::R8G8B8A8_SNORM,			// VK_FORMAT_R8G8B8A8_SNORM = 38,
	GrafFormat::Unsupported,			// VK_FORMAT_R8G8B8A8_USCALED = 39,
	GrafFormat::Unsupported,			// VK_FORMAT_R8G8B8A8_SSCALED = 40,
	GrafFormat::R8G8B8A8_UINT,			// VK_FORMAT_R8G8B8A8_UINT = 41,
	GrafFormat::R8G8B8A8_SINT,			// VK_FORMAT_R8G8B8A8_SINT = 42,
	GrafFormat::R8G8B8A8_SRGB,			// VK_FORMAT_R8G8B8A8_SRGB = 43,
	GrafFormat::B8G8R8A8_UNORM,			// VK_FORMAT_B8G8R8A8_UNORM = 44,
	GrafFormat::B8G8R8A8_SNORM,			// VK_FORMAT_B8G8R8A8_SNORM = 45,
	GrafFormat::Unsupported,			// VK_FORMAT_B8G8R8A8_USCALED = 46,
	GrafFormat::Unsupported,			// VK_FORMAT_B8G8R8A8_SSCALED = 47,
	GrafFormat::B8G8R8A8_UINT,			// VK_FORMAT_B8G8R8A8_UINT = 48,
	GrafFormat::B8G8R8A8_SINT,			// VK_FORMAT_B8G8R8A8_SINT = 49,
	GrafFormat::B8G8R8A8_SRGB,			// VK_FORMAT_B8G8R8A8_SRGB = 50,
	GrafFormat::Unsupported,			// VK_FORMAT_A8B8G8R8_UNORM_PACK32 = 51,
	GrafFormat::Unsupported,			// VK_FORMAT_A8B8G8R8_SNORM_PACK32 = 52,
	GrafFormat::Unsupported,			// VK_FORMAT_A8B8G8R8_USCALED_PACK32 = 53,
	GrafFormat::Unsupported,			// VK_FORMAT_A8B8G8R8_SSCALED_PACK32 = 54,
	GrafFormat::Unsupported,			// VK_FORMAT_A8B8G8R8_UINT_PACK32 = 55,
	GrafFormat::Unsupported,			// VK_FORMAT_A8B8G8R8_SINT_PACK32 = 56,
	GrafFormat::Unsupported,			// VK_FORMAT_A8B8G8R8_SRGB_PACK32 = 57,
	GrafFormat::Unsupported,			// VK_FORMAT_A2R10G10B10_UNORM_PACK32 = 58,
	GrafFormat::Unsupported,			// VK_FORMAT_A2R10G10B10_SNORM_PACK32 = 59,
	GrafFormat::Unsupported,			// VK_FORMAT_A2R10G10B10_USCALED_PACK32 = 60,
	GrafFormat::Unsupported,			// VK_FORMAT_A2R10G10B10_SSCALED_PACK32 = 61,
	GrafFormat::Unsupported,			// VK_FORMAT_A2R10G10B10_UINT_PACK32 = 62,
	GrafFormat::Unsupported,			// VK_FORMAT_A2R10G10B10_SINT_PACK32 = 63,
	GrafFormat::Unsupported,			// VK_FORMAT_A2B10G10R10_UNORM_PACK32 = 64,
	GrafFormat::Unsupported,			// VK_FORMAT_A2B10G10R10_SNORM_PACK32 = 65,
	GrafFormat::Unsupported,			// VK_FORMAT_A2B10G10R10_USCALED_PACK32 = 66,
	GrafFormat::Unsupported,			// VK_FORMAT_A2B10G10R10_SSCALED_PACK32 = 67,
	GrafFormat::Unsupported,			// VK_FORMAT_A2B10G10R10_UINT_PACK32 = 68,
	GrafFormat::Unsupported,			// VK_FORMAT_A2B10G10R10_SINT_PACK32 = 69,
	GrafFormat::Unsupported,			// VK_FORMAT_R16_UNORM = 70,
	GrafFormat::Unsupported,			// VK_FORMAT_R16_SNORM = 71,
	GrafFormat::Unsupported,			// VK_FORMAT_R16_USCALED = 72,
	GrafFormat::Unsupported,			// VK_FORMAT_R16_SSCALED = 73,
	GrafFormat::Unsupported,			// VK_FORMAT_R16_UINT = 74,
	GrafFormat::Unsupported,			// VK_FORMAT_R16_SINT = 75,
	GrafFormat::Unsupported,			// VK_FORMAT_R16_SFLOAT = 76,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16_UNORM = 77,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16_SNORM = 78,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16_USCALED = 79,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16_SSCALED = 80,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16_UINT = 81,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16_SINT = 82,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16_SFLOAT = 83,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16_UNORM = 84,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16_SNORM = 85,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16_USCALED = 86,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16_SSCALED = 87,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16_UINT = 88,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16_SINT = 89,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16_SFLOAT = 90,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16A16_UNORM = 91,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16A16_SNORM = 92,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16A16_USCALED = 93,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16A16_SSCALED = 94,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16A16_UINT = 95,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16A16_SINT = 96,
	GrafFormat::Unsupported,			// VK_FORMAT_R16G16B16A16_SFLOAT = 97,
	GrafFormat::Unsupported,			// VK_FORMAT_R32_UINT = 98,
	GrafFormat::Unsupported,			// VK_FORMAT_R32_SINT = 99,
	GrafFormat::R32_SFLOAT,				// VK_FORMAT_R32_SFLOAT = 100,
	GrafFormat::Unsupported,			// VK_FORMAT_R32G32_UINT = 101,
	GrafFormat::Unsupported,			// VK_FORMAT_R32G32_SINT = 102,
	GrafFormat::R32G32_SFLOAT,			// VK_FORMAT_R32G32_SFLOAT = 103,
	GrafFormat::Unsupported,			// VK_FORMAT_R32G32B32_UINT = 104,
	GrafFormat::Unsupported,			// VK_FORMAT_R32G32B32_SINT = 105,
	GrafFormat::R32G32B32_SFLOAT,		// VK_FORMAT_R32G32B32_SFLOAT = 106,
	GrafFormat::Unsupported,			// VK_FORMAT_R32G32B32A32_UINT = 107,
	GrafFormat::Unsupported,			// VK_FORMAT_R32G32B32A32_SINT = 108,
	GrafFormat::R32G32B32A32_SFLOAT,	// VK_FORMAT_R32G32B32A32_SFLOAT = 109,
	GrafFormat::Unsupported,			// VK_FORMAT_R64_UINT = 110,
	GrafFormat::Unsupported,			// VK_FORMAT_R64_SINT = 111,
	GrafFormat::Unsupported,			// VK_FORMAT_R64_SFLOAT = 112,
	GrafFormat::Unsupported,			// VK_FORMAT_R64G64_UINT = 113,
	GrafFormat::Unsupported,			// VK_FORMAT_R64G64_SINT = 114,
	GrafFormat::Unsupported,			// VK_FORMAT_R64G64_SFLOAT = 115,
	GrafFormat::Unsupported,			// VK_FORMAT_R64G64B64_UINT = 116,
	GrafFormat::Unsupported,			// VK_FORMAT_R64G64B64_SINT = 117,
	GrafFormat::Unsupported,			// VK_FORMAT_R64G64B64_SFLOAT = 118,
	GrafFormat::Unsupported,			// VK_FORMAT_R64G64B64A64_UINT = 119,
	GrafFormat::Unsupported,			// VK_FORMAT_R64G64B64A64_SINT = 120,
	GrafFormat::Unsupported,			// VK_FORMAT_R64G64B64A64_SFLOAT = 121,
	GrafFormat::Unsupported,			// VK_FORMAT_B10G11R11_UFLOAT_PACK32 = 122,
	GrafFormat::Unsupported,			// VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 = 123,
	GrafFormat::Unsupported,			// VK_FORMAT_D16_UNORM = 124,
	GrafFormat::Unsupported,			// VK_FORMAT_X8_D24_UNORM_PACK32 = 125,
	GrafFormat::Unsupported,			// VK_FORMAT_D32_SFLOAT = 126,
	GrafFormat::Unsupported,			// VK_FORMAT_S8_UINT = 127,
	GrafFormat::Unsupported,			// VK_FORMAT_D16_UNORM_S8_UINT = 128,
	GrafFormat::Unsupported,			// VK_FORMAT_D24_UNORM_S8_UINT = 129,
	GrafFormat::Unsupported,			// VK_FORMAT_D32_SFLOAT_S8_UINT = 130,
	GrafFormat::Unsupported,			// VK_FORMAT_BC1_RGB_UNORM_BLOCK = 131,
	GrafFormat::Unsupported,			// VK_FORMAT_BC1_RGB_SRGB_BLOCK = 132,
	GrafFormat::Unsupported,			// VK_FORMAT_BC1_RGBA_UNORM_BLOCK = 133,
	GrafFormat::Unsupported,			// VK_FORMAT_BC1_RGBA_SRGB_BLOCK = 134,
	GrafFormat::Unsupported,			// VK_FORMAT_BC2_UNORM_BLOCK = 135,
	GrafFormat::Unsupported,			// VK_FORMAT_BC2_SRGB_BLOCK = 136,
	GrafFormat::Unsupported,			// VK_FORMAT_BC3_UNORM_BLOCK = 137,
	GrafFormat::Unsupported,			// VK_FORMAT_BC3_SRGB_BLOCK = 138,
	GrafFormat::Unsupported,			// VK_FORMAT_BC4_UNORM_BLOCK = 139,
	GrafFormat::Unsupported,			// VK_FORMAT_BC4_SNORM_BLOCK = 140,
	GrafFormat::Unsupported,			// VK_FORMAT_BC5_UNORM_BLOCK = 141,
	GrafFormat::Unsupported,			// VK_FORMAT_BC5_SNORM_BLOCK = 142,
	GrafFormat::Unsupported,			// VK_FORMAT_BC6H_UFLOAT_BLOCK = 143,
	GrafFormat::Unsupported,			// VK_FORMAT_BC6H_SFLOAT_BLOCK = 144,
	GrafFormat::Unsupported,			// VK_FORMAT_BC7_UNORM_BLOCK = 145,
	GrafFormat::Unsupported,			// VK_FORMAT_BC7_SRGB_BLOCK = 146,
	GrafFormat::Unsupported,			// VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK = 147,
	GrafFormat::Unsupported,			// VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK = 148,
	GrafFormat::Unsupported,			// VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK = 149,
	GrafFormat::Unsupported,			// VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK = 150,
	GrafFormat::Unsupported,			// VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK = 151,
	GrafFormat::Unsupported,			// VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK = 152,
	GrafFormat::Unsupported,			// VK_FORMAT_EAC_R11_UNORM_BLOCK = 153,
	GrafFormat::Unsupported,			// VK_FORMAT_EAC_R11_SNORM_BLOCK = 154,
	GrafFormat::Unsupported,			// VK_FORMAT_EAC_R11G11_UNORM_BLOCK = 155,
	GrafFormat::Unsupported,			// VK_FORMAT_EAC_R11G11_SNORM_BLOCK = 156,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_4x4_UNORM_BLOCK = 157,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_4x4_SRGB_BLOCK = 158,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_5x4_UNORM_BLOCK = 159,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_5x4_SRGB_BLOCK = 160,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_5x5_UNORM_BLOCK = 161,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_5x5_SRGB_BLOCK = 162,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_6x5_UNORM_BLOCK = 163,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_6x5_SRGB_BLOCK = 164,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_6x6_UNORM_BLOCK = 165,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_6x6_SRGB_BLOCK = 166,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_8x5_UNORM_BLOCK = 167,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_8x5_SRGB_BLOCK = 168,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_8x6_UNORM_BLOCK = 169,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_8x6_SRGB_BLOCK = 170,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_8x8_UNORM_BLOCK = 171,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_8x8_SRGB_BLOCK = 172,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_10x5_UNORM_BLOCK = 173,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_10x5_SRGB_BLOCK = 174,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_10x6_UNORM_BLOCK = 175,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_10x6_SRGB_BLOCK = 176,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_10x8_UNORM_BLOCK = 177,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_10x8_SRGB_BLOCK = 178,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_10x10_UNORM_BLOCK = 179,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_10x10_SRGB_BLOCK = 180,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_12x10_UNORM_BLOCK = 181,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_12x10_SRGB_BLOCK = 182,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_12x12_UNORM_BLOCK = 183,
	GrafFormat::Unsupported,			// VK_FORMAT_ASTC_12x12_SRGB_BLOCK = 184,
};
GrafFormat GrafUtilsVulkan::VkToGrafFormat(VkFormat vkFormat)
{
	return VkToGrafFormatLUT[ur_uint(vkFormat)];
}