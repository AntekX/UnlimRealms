
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

		res = grafSystem->CreateDescriptorTableLayout(this->graphicsObjects->workGraphDescTableLayout);
		if (Succeeded(res))
		{
			GrafDescriptorRangeDesc descTableLayoutRanges[] = {
				g_UAVDescriptor
			};
			GrafDescriptorTableLayoutDesc descTableLayoutDesc = {
				ur_uint(GrafShaderStageFlag::Compute),
				descTableLayoutRanges, ur_array_size(descTableLayoutRanges)
			};
			res = this->graphicsObjects->workGraphDescTableLayout->Initialize(grafDevice, { descTableLayoutDesc });
		}
		if (Failed(res))
			break;

		this->graphicsObjects->workGraphDescTable.reset(new GrafManagedDescriptorTable(*this->GetGrafRenderer()));
		res = this->graphicsObjects->workGraphDescTable->Initialize({ this->graphicsObjects->workGraphDescTableLayout.get() });
		if (Failed(res))
			break;

		res = grafSystem->CreateWorkGraphPipeline(this->graphicsObjects->workGraphPipeline);
		if (Succeeded(res))
		{
			GrafDescriptorTableLayout* workGraphDescriptorLayouts[] = {
				this->graphicsObjects->workGraphDescTableLayout.get()
			};
			GrafWorkGraphPipeline::InitParams initParams = {};
			initParams.Name = "HelloWorkGraphs";
			initParams.ShaderLib = this->graphicsObjects->workGraphShaderLib.get();
			initParams.DescriptorTableLayouts = workGraphDescriptorLayouts;
			initParams.DescriptorTableLayoutCount = ur_array_size(workGraphDescriptorLayouts);
			res = this->graphicsObjects->workGraphPipeline->Initialize(grafDevice, initParams);
		}
		if (Failed(res))
			break;

		res = grafSystem->CreateBuffer(this->graphicsObjects->workGraphDataBuffer);
		if (Succeeded(res))
		{
			GrafBuffer::InitParams bufferParams = {};
			bufferParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::StorageBuffer) | ur_uint(GrafBufferUsageFlag::TransferDst);
			bufferParams.BufferDesc.MemoryType = ur_uint(GrafDeviceMemoryFlag::GpuLocal);
			bufferParams.BufferDesc.SizeInBytes = 16777216 * sizeof(ur_uint32);
			res = this->graphicsObjects->workGraphDataBuffer->Initialize(grafDevice, bufferParams);
			
			// TODO: upload initial data
			//this->GetGrafRenderer()->Upload((ur_byte*)intialData, this->graphicsObjects->workGworkGraphDataBufferraphBuffer.get(), bufferParams.BufferDesc.SizeInBytes);
		}
		if (Failed(res))
			break;

	} while (false);
	if (Failed(res))
	{
		this->DeinitializeGraphicObjects();
		return ResultError(Failure, "GPUWorkGraphsRealm: failed to initialize graphic objects");
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
	if (ur_null == this->graphicsObjects.get())
		return Result(NotInitialized);

	// prepare resources
	renderContext.CommandList->BufferMemoryBarrier(this->graphicsObjects->workGraphDataBuffer.get(), GrafBufferState::Current, GrafBufferState::ComputeReadWrite);

	// update descriptor table
	GrafDescriptorTable* workGraphFrameTable = this->graphicsObjects->workGraphDescTable->GetFrameObject();
	//workGraphFrameTable->SetRWBuffer(g_UAVDescriptor, this->graphicsObjects->workGraphDataBuffer.get());

	// bind pipeline
	renderContext.CommandList->BindWorkGraphPipeline(this->graphicsObjects->workGraphPipeline.get());
	renderContext.CommandList->BindWorkGraphDescriptorTable(workGraphFrameTable, this->graphicsObjects->workGraphPipeline.get());

	// TODO: dispatch work graph

	return Result(Success);
}