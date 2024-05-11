
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
#if (UR_GRAF_DX12_WORK_GRAPHS_SUPPORTED)

	RenderRealm::InitParams realmParams = RenderRealm::InitParams::Default;
	realmParams.Title = L"GPUWorkGraphsApp";
	realmParams.CanvasStyle = WinCanvas::Style::OverlappedWindowMaximized;
	realmParams.GrafSystemType = RenderRealm::GrafSystemType::DX12;
	realmParams.RendererParams = GrafRenderer::InitParams::Default;

	return RenderRealm::Initialize(realmParams);
#else
	return Result(NotImplemented); // used DX12 SDK does not support work graphs
#endif
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
				g_CBDescriptor,
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
			bufferParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::StorageBuffer) | ur_uint(GrafBufferUsageFlag::TransferSrc);
			bufferParams.BufferDesc.MemoryType = ur_uint(GrafDeviceMemoryFlag::GpuLocal);
			bufferParams.BufferDesc.SizeInBytes = 16777216 * sizeof(ur_uint32);
			res = this->graphicsObjects->workGraphDataBuffer->Initialize(grafDevice, bufferParams);
			
			// upload initial data
			std::vector<ur_uint32> initialData(16777216, 0);
			this->GetGrafRenderer()->Upload((ur_byte*)initialData.data(), this->graphicsObjects->workGraphDataBuffer.get(), bufferParams.BufferDesc.SizeInBytes);
		}
		if (Failed(res))
			break;

		res = grafSystem->CreateBuffer(this->graphicsObjects->workGraphReadbackBuffer);
		if (Succeeded(res))
		{
			GrafBuffer::InitParams bufferParams = {};
			bufferParams.BufferDesc.Usage = ur_uint(GrafBufferUsageFlag::TransferDst);
			bufferParams.BufferDesc.MemoryType = ur_uint(GrafDeviceMemoryFlag::CpuVisible);
			bufferParams.BufferDesc.SizeInBytes = 16777216 * sizeof(ur_uint32);
			res = this->graphicsObjects->workGraphReadbackBuffer->Initialize(grafDevice, bufferParams);
			this->graphicsObjects->workGraphReadbackPending = false;
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

	if (this->graphicsObjects->workGraphReadbackPending)
		return Result(Success); // do not dispatch another work graph till readback is done

	// prepare resources
	renderContext.CommandList->BufferMemoryBarrier(this->graphicsObjects->workGraphDataBuffer.get(), GrafBufferState::Current, GrafBufferState::ComputeReadWrite);

	// update descriptor table
	GrafDescriptorTable* workGraphFrameTable = this->graphicsObjects->workGraphDescTable->GetFrameObject();
	workGraphFrameTable->SetRWBuffer(g_UAVDescriptor, this->graphicsObjects->workGraphDataBuffer.get());

	// bind pipeline
	renderContext.CommandList->BindWorkGraphPipeline(this->graphicsObjects->workGraphPipeline.get());
	renderContext.CommandList->BindWorkGraphDescriptorTable(workGraphFrameTable, this->graphicsObjects->workGraphPipeline.get());

	// generate graph inputs
	std::vector<entryRecord> graphInputData;
	UINT numRecords = 4;
	graphInputData.resize(numRecords);
	for (UINT recordIndex = 0; recordIndex < numRecords; recordIndex++)
	{
		graphInputData[recordIndex].gridSize = recordIndex + 1;
		graphInputData[recordIndex].recordIndex = recordIndex;
	}

	// dispatch work graph
	renderContext.CommandList->DispatchGraph(0, (ur_byte*)graphInputData.data(), (ur_uint)graphInputData.size(), sizeof(entryRecord));

	// schedule readback
	renderContext.CommandList->BufferMemoryBarrier(this->graphicsObjects->workGraphDataBuffer.get(), GrafBufferState::Current, GrafBufferState::TransferSrc);
	renderContext.CommandList->Copy(this->graphicsObjects->workGraphDataBuffer.get(), this->graphicsObjects->workGraphReadbackBuffer.get());
	this->graphicsObjects->workGraphReadbackPending = true;

	// retrieve data fro UAV when work graph executed
	// TODO: redo, callback can be executed asynchronously
	this->GetGrafRenderer()->AddCommandListCallback(renderContext.CommandList, {},
		[this](GrafCallbackContext& ctx)->Result
		{
			this->graphicsObjects->workGraphReadbackData.resize(16777216);
			ur_byte* readbackDataPtr = (ur_byte*)this->graphicsObjects->workGraphReadbackData.data();
			this->graphicsObjects->workGraphReadbackBuffer->Read(readbackDataPtr);
			this->graphicsObjects->workGraphReadbackPending = false;
			return Result(Success);
		}
	);

	return Result(Success);
}