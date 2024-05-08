
#include "stdafx.h"
#include "GPUWorkGraphsRealm.h"
#include "UnlimRealms.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"
#include "Sys/JobSystem.h"
#include "Sys/Windows/WinCanvas.h"
#include "Sys/Windows/WinInput.h"
#include "Core/Math.h"
#include "Graf/GrafRenderer.h"
#include "Graf/Vulkan/GrafSystemVulkan.h"
#include "Graf/DX12/GrafSystemDX12.h"
#include "GPUWorkGraphsCommon.h"
#pragma comment(lib, "UnlimRealms.lib")

using namespace UnlimRealms;

#define UR_GRAF_SYSTEM GrafSystemDX12

GPUWorkGraphsRealm::GPUWorkGraphsRealm()
{
}

GPUWorkGraphsRealm::~GPUWorkGraphsRealm()
{
}

Result GPUWorkGraphsRealm::Initialize()
{
	RenderRealm::InitParams realmParams = RenderRealm::InitParams::Default;
	realmParams.Title = L"GPUWorkGraphsApp";
	realmParams.CanvasStyle = WinCanvas::Style::OverlappedWindowMaximized;
	realmParams.GrafSystemType = RenderRealm::GrafSystemType::DX12;
	realmParams.RendererParams = GrafRenderer::InitParams::Default;

	return RenderRealm::Initialize(realmParams);
}

Result GPUWorkGraphsRealm::InitializeGraphicObjects()
{
	GrafSystem* grafSystem = this->GetGrafRenderer()->GetGrafSystem();
	GrafDevice* grafDevice = this->GetGrafRenderer()->GetGrafDevice();
	this->graphicsObjects.reset(new GraphicsObjects());
	
	Result res = Success;
	do
	{
		res = grafSystem->CreateShaderLib(this->graphicsObjects->workGraphShaderLib);
		if (Succeeded(res))
		{
			GrafShaderLib::EntryPoint ShaderLibEntries[] = {
				{ "firstNode", GrafShaderType::Node }
			};
			res = GrafUtils::CreateShaderLibFromFile(*grafDevice, "GPUWorkGraphs_cs_lib", ShaderLibEntries, ur_array_size(ShaderLibEntries), this->graphicsObjects->workGraphShaderLib);
		}
		if (Failed(res))
			break;

		res = grafSystem->CreateWorkGraphPipeline(this->graphicsObjects->workGraphPipeline);
		if (Succeeded(res))
		{
			GrafWorkGraphPipeline::InitParams initParams = {};
			initParams.Name = "HelloWorkGraphs";
			initParams.ShaderLib = this->graphicsObjects->workGraphShaderLib.get();
			initParams.DescriptorTableLayouts = ur_null;
			initParams.DescriptorTableLayoutCount = 0;
			res = this->graphicsObjects->workGraphPipeline->Initialize(grafDevice, initParams);
		}
		if (Failed(res))
			break;

	} while (false);
	if (Failed(res))
	{
		this->DeinitializeGraphicObjects();
		LogError("GPUWorkGraphsRealm: failed to initialize graphic objects");
		return Result(Failure);
	}

	return Result(Success);
}

Result GPUWorkGraphsRealm::DeinitializeGraphicObjects()
{
	this->graphicsObjects.reset(ur_null);

	return Result(Success);
}

Result GPUWorkGraphsRealm::Update(const UpdateContext& updateContext)
{
	return Result(Success);
}

Result GPUWorkGraphsRealm::Render(const RenderContext& renderContext)
{
	return Result(Success);
}