
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
#include "ImguiRender/ImguiRender.h"
#include "GenericRender/GenericRender.h"
#include "Camera/CameraControl.h"

#include "Graf/Vulkan/GrafSystemVulkan.h"
#include "Graf/DX12/GrafSystemDX12.h"
#include "HDRRender/HDRRender.h"
#include "Atmosphere/Atmosphere.h"
#include "HybridRenderingCommon.h"
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
	return Result(NotImplemented);
}

Result GPUWorkGraphsRealm::DeinitializeGraphicObjects()
{
	return Result(NotImplemented);
}

Result GPUWorkGraphsRealm::Update(const UpdateContext& updateContext)
{
	return Result(NotImplemented);
}

Result GPUWorkGraphsRealm::Render(const RenderContext& renderContext)
{
	return Result(NotImplemented);
}