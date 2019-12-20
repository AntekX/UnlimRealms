
#include "stdafx.h"
#include "VulkanSandboxApp.h"

#include "UnlimRealms.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"
#include "Sys/JobSystem.h"
#include "Sys/Windows/WinCanvas.h"
#include "Sys/Windows/WinInput.h"
#include "Graf/Vulkan/GrafSystemVulkan.h"
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

	// sample mesh data
	struct VertexSample
	{
		ur_float3 pos;
		ur_float3 color;
	};
	VertexSample verticesSample[] = {
		{ {-0.5f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
		{ { 0.5f,-0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
		{ {-0.5f,-0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
		{ {-0.5f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
		{ { 0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
		{ { 0.5f,-0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } }
	};

	// sample vertex shader CB
	struct SampleCBData
	{
		ur_float4x4 Transform;
	};

	// initialize gfx system
	std::unique_ptr<GrafSystem> grafSystem(new GrafSystemVulkan(realm));
	std::unique_ptr<GrafDevice> grafDevice;
	std::unique_ptr<GrafCanvas> grafCanvas;
	std::unique_ptr<GrafSampler> grafDefaultSampler;
	std::unique_ptr<GrafShader> grafShaderSampleVS;
	std::unique_ptr<GrafShader> grafShaderSamplePS;
	std::unique_ptr<GrafBuffer> grafVBSample;
	std::unique_ptr<GrafImage> grafImageSample;
	std::unique_ptr<GrafCommandList> grafUploadCmdList;
	std::unique_ptr<GrafUtils::ImageData> sampleImageData;
	std::unique_ptr<GrafRenderPass> grafRenderPassSample;
	std::unique_ptr<GrafDescriptorTableLayout> grafBindingLayoutSample;
	std::unique_ptr<GrafPipeline> grafPipelineSample;
	std::vector<std::unique_ptr<GrafRenderTarget>> grafRenderTarget;
	std::vector<std::unique_ptr<GrafCommandList>> grafMainCmdList;
	std::vector<std::unique_ptr<GrafBuffer>> grafCBSample;
	std::vector<std::unique_ptr<GrafDescriptorTable>> grafBindingSample;
	ur_uint frameCount = 0;
	ur_uint frameIdx = 0;
	auto& deinitializeGfxCanvasObjects = [&]() -> void {
		if (grafDevice.get()) grafDevice->WaitIdle();
		grafRenderTarget.clear();
	};
	auto& deinitializeGfxFrameObjects = [&]() -> void {
		grafBindingSample.clear();
		grafMainCmdList.clear();
		grafCBSample.clear();
	};
	auto& deinitializeGfxSystem = [&]() -> void {
		// order matters!
		deinitializeGfxCanvasObjects();
		deinitializeGfxFrameObjects();
		grafUploadCmdList.reset();
		grafPipelineSample.reset();
		grafRenderPassSample.reset();
		grafVBSample.reset();
		grafImageSample.reset();
		sampleImageData.reset();
		grafBindingLayoutSample.reset();
		grafShaderSampleVS.reset();
		grafShaderSamplePS.reset();
		grafDefaultSampler.reset();
		grafCanvas.reset();
		grafDevice.reset();
		grafSystem.reset();
	};
	Result grafRes = NotInitialized;
	auto& initializeGfxSystem = [&]() -> void
	{
		// system
		grafRes = grafSystem->Initialize(realm.GetCanvas());
		if (Failed(grafRes) || 0 == grafSystem->GetPhysicalDeviceCount()) return;
		
		// device
		grafRes = grafSystem->CreateDevice(grafDevice);
		if (Failed(grafRes)) return;
		grafRes = grafDevice->Initialize(grafSystem->GetRecommendedDeviceId());
		if (Failed(grafRes)) return;

		// presentation canvas (swapchain)
		grafRes = grafSystem->CreateCanvas(grafCanvas);
		if (Failed(grafRes)) return;
		grafRes = grafCanvas->Initialize(grafDevice.get(), GrafCanvas::InitParams::Default);
		if (Failed(grafRes)) return;

		// color render target pass
		grafRes = grafSystem->CreateRenderPass(grafRenderPassSample);
		if (Failed(grafRes)) return;
		grafRes = grafRenderPassSample->Initialize(grafDevice.get());
		if (Failed(grafRes)) return;

		// default sampler
		grafRes = grafSystem->CreateSampler(grafDefaultSampler);
		if (Failed(grafRes)) return;
		grafRes = grafDefaultSampler->Initialize(grafDevice.get(), { GrafSamplerDesc::Default });
		if (Failed(grafRes)) return;

		// vertex shader sample
		grafRes = GrafUtils::CreateShaderFromFile(*grafDevice.get(), "sample_vs.spv", GrafShaderType::Vertex, grafShaderSampleVS);
		if (Failed(grafRes)) return;

		// pixel shader sample
		grafRes = GrafUtils::CreateShaderFromFile(*grafDevice.get(), "sample_ps.spv", GrafShaderType::Pixel, grafShaderSamplePS);
		if (Failed(grafRes)) return;

		// shader bindings layout sample
		grafRes = grafSystem->CreateDescriptorTableLayout(grafBindingLayoutSample);
		if (Failed(grafRes)) return;
		GrafDescriptorRangeDesc grafBindingLayoutSampleRanges[] = {
			{ GrafDescriptorType::ConstantBuffer, 0, 1 },
			{ GrafDescriptorType::Sampler, 0, 1 },
			{ GrafDescriptorType::Texture, 0, 1 },
		};
		GrafDescriptorTableLayoutDesc grafBindingLayoutSampleDesc = {
			GrafShaderStageFlags((ur_uint)GrafShaderStageFlag::Vertex | (ur_uint)GrafShaderStageFlag::Pixel),
			grafBindingLayoutSampleRanges, ur_array_size(grafBindingLayoutSampleRanges)
		};
		grafRes = grafBindingLayoutSample->Initialize(grafDevice.get(), { grafBindingLayoutSampleDesc });
		if (Failed(grafRes)) return;

		// vertex buffer sample
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

		// graphics pipeline for sample rendering
		grafRes = grafSystem->CreatePipeline(grafPipelineSample);
		if (Failed(grafRes)) return;
		GrafShader* sampleShaderStages[] = {
			grafShaderSampleVS.get(),
			grafShaderSamplePS.get()
		};
		GrafDescriptorTableLayout* sampleBindingLayouts[] = {
			grafBindingLayoutSample.get(),
		};
		GrafVertexElementDesc sampleVertexElements[] = {
			{ GrafFormat::R32G32B32_SFLOAT, 0 },
			{ GrafFormat::R32G32B32_SFLOAT, 12 }
		};
		GrafVertexInputDesc sampleVertexInputs[] = {{
			GrafVertexInputType::PerVertex, 0, sizeof(VertexSample),
			sampleVertexElements, ur_array_size(sampleVertexElements)
		}};
		GrafPipeline::InitParams samplePipelineParams = GrafPipeline::InitParams::Default;
		samplePipelineParams.RenderPass = grafRenderPassSample.get();
		samplePipelineParams.ShaderStages = sampleShaderStages;
		samplePipelineParams.ShaderStageCount = ur_array_size(sampleShaderStages);
		samplePipelineParams.DescriptorTableLayouts = sampleBindingLayouts;
		samplePipelineParams.DescriptorTableLayoutCount = ur_array_size(sampleBindingLayouts);
		samplePipelineParams.VertexInputDesc = sampleVertexInputs;
		samplePipelineParams.VertexInputCount = ur_array_size(sampleVertexInputs);
		grafRes = grafPipelineSample->Initialize(grafDevice.get(), samplePipelineParams);
		if (Failed(grafRes)) return;
	};
	auto& initializeGfxFrameObjects = [&]() -> void
	{
		if (Failed(grafRes)) return;

		// number of recorded (in flight) frames
		// can differ from swap chain size
		frameCount = std::max(ur_uint(1), grafCanvas->GetSwapChainImageCount() - 1);
		frameIdx = 0;

		// command lists (per frame)
		grafMainCmdList.resize(frameCount);
		for (ur_uint iframe = 0; iframe < frameCount; ++iframe)
		{
			grafRes = grafSystem->CreateCommandList(grafMainCmdList[iframe]);
			if (Failed(grafRes)) break;
			grafRes = grafMainCmdList[iframe]->Initialize(grafDevice.get());
			if (Failed(grafRes)) break;
		}
		if (Failed(grafRes)) return;

		// sample shader decriptor tables (per frame & per draw call)
		// tables share the same layout but store unique per frame/call data
		grafBindingSample.resize(frameCount);
		for (ur_uint iframe = 0; iframe < frameCount; ++iframe)
		{
			grafRes = grafSystem->CreateDescriptorTable(grafBindingSample[iframe]);
			if (Failed(grafRes)) break;
			grafRes = grafBindingSample[iframe]->Initialize(grafDevice.get(), { grafBindingLayoutSample.get() });
			if (Failed(grafRes)) break;
		}
		if (Failed(grafRes)) return;

		// sample shader constant buffer (per frame & per draw call)
		GrafBuffer::InitParams grafCBSampleParams;
		grafCBSampleParams.BufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::ConstantBuffer;
		grafCBSampleParams.BufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::CpuVisible;
		grafCBSampleParams.BufferDesc.SizeInBytes = sizeof(SampleCBData);
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
	auto& initializeGfxCanvasObjects = [&]() -> void
	{
		if (Failed(grafRes)) return;

		// render targets
		// RT count must be equal to swap chain size (one RT wraps on swap chain image)
		grafRenderTarget.resize(grafCanvas->GetSwapChainImageCount());
		for (ur_uint imageIdx = 0; imageIdx < grafCanvas->GetSwapChainImageCount(); ++imageIdx)
		{
			grafRes = grafSystem->CreateRenderTarget(grafRenderTarget[imageIdx]);
			if (Failed(grafRes)) break;
			GrafImage* renderTargetImages[] = {
				grafCanvas->GetSwapChainImage(imageIdx)
			};
			GrafRenderTarget::InitParams renderTargetParams = {};
			renderTargetParams.RenderPass = grafRenderPassSample.get();
			renderTargetParams.Images = renderTargetImages;
			renderTargetParams.ImageCount = ur_array_size(renderTargetImages);
			grafRes = grafRenderTarget[imageIdx]->Initialize(grafDevice.get(), renderTargetParams);
			if (Failed(grafRes)) break;
		}
		if (Failed(grafRes)) return;
	};
	initializeGfxSystem();
	initializeGfxFrameObjects();
	initializeGfxCanvasObjects();
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

	// prepare sample image
	if (grafSystem != ur_null)
	{
		do
		{
			// load from file to cpu visible buffer(s)
			sampleImageData.reset(new GrafUtils::ImageData());
			grafRes = GrafUtils::LoadImageFromFile(*grafDevice.get(), "../Res/testimage.dds", *sampleImageData.get());
			if (Failed(grafRes)) break;
			
			// create image in gpu local memory
			grafRes = grafSystem->CreateImage(grafImageSample);
			if (Failed(grafRes)) break;
			GrafImageDesc grafImageDesc = sampleImageData->Desc;
			grafImageDesc.MipLevels = 1; // TODO: fill all mips
			grafRes = grafImageSample->Initialize(grafDevice.get(), { grafImageDesc });
			if (Failed(grafRes)) break;

			// init upload command list
			grafRes = grafSystem->CreateCommandList(grafUploadCmdList);
			if (Failed(grafRes)) break;
			grafRes = grafUploadCmdList->Initialize(grafDevice.get());
			if (Failed(grafRes)) break;

			// submit commands performing required image memory transitions and cpu->gpu memory copies
			grafUploadCmdList->Begin();
			grafUploadCmdList->ImageMemoryBarrier(grafImageSample.get(), GrafImageState::Current, GrafImageState::TransferDst);
			grafUploadCmdList->Copy(sampleImageData->MipBuffers[0].get(), grafImageSample.get());
			grafUploadCmdList->ImageMemoryBarrier(grafImageSample.get(), GrafImageState::Current, GrafImageState::ShaderRead);
			grafUploadCmdList->End();
			grafDevice->Submit(grafUploadCmdList.get());
		}
		while (false);
		if (Failed(grafRes))
		{
			sampleImageData.reset();
			grafImageSample.reset();
		}
	}

	// initialize ImguiRender
	ImguiRender* imguiRender = ur_null;
	if (grafSystem != ur_null)
	{
		imguiRender = realm.AddComponent<ImguiRender>(realm);
		Result res = imguiRender->Init(*grafDevice.get(), *grafRenderPassSample.get(), frameCount);
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
	ur_float2 sampleAnimPos[4];

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
				grafCanvas->Initialize(grafDevice.get(), GrafCanvas::InitParams::Default);
				deinitializeGfxCanvasObjects();
				initializeGfxCanvasObjects();
			}
		}
		if (!canvasValid)
			Sleep(60);

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
			for (ur_uint instId = 0; instId < 4; ++instId)
			{
				ur_float animAngle = sampleAnimElapsedTime / (sampleAnimDelay * (instId + 1)) * MathConst<ur_float>::Pi * 2.0f + instId * MathConst<ur_float>::Pi / 8;
				sampleAnimPos[instId].x = cosf(animAngle) * sampleAnimRadius;
				sampleAnimPos[instId].y = sinf(animAngle) * sampleAnimRadius;
			}
			ctx.progress = 2;
		});

		// draw frame
		if (grafSystem != ur_null && canvasValid)
		{
			auto drawFrameJob = realm.GetJobSystem().Add(ur_null, [&](Job::Context& ctx) -> void {

				// updateFrameJob->Wait(); // wait till update job is fully finished; WaitProgress can be used instead to wait for specific update stage to avoid stalling draw thread

				GrafCommandList* grafCmdListCrnt = grafMainCmdList[frameIdx].get();
				grafCmdListCrnt->Begin();

				// prepare RT

				updateFrameJob->WaitProgress(1); // make sure required data is ready
				grafCmdListCrnt->ClearColorImage(grafCanvas->GetCurrentImage(), reinterpret_cast<GrafClearValue&>(crntClearColor));
				grafCmdListCrnt->BeginRenderPass(grafRenderPassSample.get(), grafRenderTarget[grafCanvas->GetCurrentImageId()].get());

				GrafViewportDesc grafViewport = {};
				grafViewport.Width = (ur_float)grafCanvas->GetCurrentImage()->GetDesc().Size.x;
				grafViewport.Height = (ur_float)grafCanvas->GetCurrentImage()->GetDesc().Size.y;
				grafViewport.Near = 0.0f;
				grafViewport.Far = 1.0f;
				grafCmdListCrnt->SetViewport(grafViewport, true);
				grafCmdListCrnt->SetScissorsRect({ 0, 0, (ur_int)grafViewport.Width, (ur_int)grafViewport.Height });

				// draw sample primitives

				updateFrameJob->WaitProgress(2); // wait till animation params are up to date
				GrafPipelineVulkan* grafPipelineVulkan = static_cast<GrafPipelineVulkan*>(grafPipelineSample.get());
				SampleCBData sampleCBData;
				sampleCBData.Transform = ur_float4x4::Identity;
				for (ur_uint instId = 0; instId < 4; ++instId)
				{
					sampleCBData.Transform.r[instId].x = sampleAnimPos[instId].x;
					sampleCBData.Transform.r[instId].y = sampleAnimPos[instId].y;
				}
				sampleCBData.Transform.Transpose();
				grafCBSample[frameIdx]->Write((ur_byte*)&sampleCBData);
				grafBindingSample[frameIdx]->SetConstantBuffer(0, grafCBSample[frameIdx].get());
				grafBindingSample[frameIdx]->SetSampledImage(0, grafImageSample.get(), grafDefaultSampler.get());

				grafCmdListCrnt->BindPipeline(grafPipelineSample.get());
				grafCmdListCrnt->BindDescriptorTable(grafBindingSample[frameIdx].get(), grafPipelineSample.get());
				grafCmdListCrnt->BindVertexBuffer(grafVBSample.get(), 0);
				grafCmdListCrnt->Draw(6, 4, 0, 0);

				// draw GUI

				static const ImVec2 imguiDemoWndSize(300.0f, (float)canvasHeight);
				static bool showGUI = true;
				showGUI = (realm.GetInput()->GetKeyboard()->IsKeyReleased(Input::VKey::F1) ? !showGUI : showGUI);
				if (showGUI)
				{
					ImGui::SetNextWindowSize(imguiDemoWndSize, ImGuiSetCond_Once);
					ImGui::SetNextWindowPos({ canvasWidth - imguiDemoWndSize.x, 0.0f }, ImGuiSetCond_Once);
					ImGui::Begin("Control Panel");
					ImGui::Text("Graphics Device: %S", grafDevice->GetPhysicalDeviceDesc()->Description.c_str());
					ImGui::End();
					ImGui::SetNextWindowSize({ 0.0f, 0.0f }, ImGuiSetCond_FirstUseEver);
					ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiSetCond_Once);
					ImGui::ShowMetricsWindow();
					
					imguiRender->Render(*grafCmdListCrnt, frameIdx);
				}

				// finalize & submit

				grafCmdListCrnt->EndRenderPass();
				grafCmdListCrnt->End();
				grafDevice->Submit(grafCmdListCrnt);
			});
			
			drawFrameJob->Wait();

			// present & move to next frame
			grafCanvas->Present();

			// start recording next frame
			frameIdx = (frameIdx + 1) % frameCount;
		}
	}

	if (grafSystem != ur_null)
	{
		// make sure there are no resources still used on gpu before destroying
		grafDevice->WaitIdle();
	}

	if (imguiRender != ur_null)
	{
		realm.RemoveComponent<ImguiRender>();
	}

	deinitializeGfxSystem();

	return 0;
}