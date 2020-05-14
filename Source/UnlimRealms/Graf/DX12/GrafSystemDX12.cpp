///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "GrafSystemDX12.h"
#include "Sys/Log.h"
#if defined(_WINDOWS)
#include "Sys/Windows/WinCanvas.h"
#endif
#include "Gfx/D3D12/d3dx12.h"
#include "Gfx/DXGIUtils/DXGIUtils.h"
#include "comdef.h"
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

namespace UnlimRealms
{

	static const char* HResultToString(HRESULT res)
	{
		// TODO
		return (FAILED(res) ? "FAILED" : "SUCCEEDED");
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafSystemDX12::GrafSystemDX12(Realm &realm) : 
		GrafSystem(realm)
	{
	}

	GrafSystemDX12::~GrafSystemDX12()
	{
		this->Deinitialize();
	}

	Result GrafSystemDX12::Deinitialize()
	{
		this->dxgiFactory.reset();

		return Result(Success);
	}

	Result GrafSystemDX12::Initialize(Canvas *canvas)
	{
		this->Deinitialize();

		LogNoteGrafDbg("GrafSystemDX12: initialization...");

		GrafSystem::Initialize(canvas);

		// initiaize DXGI

		HRESULT hres = CreateDXGIFactory2(0, __uuidof(this->dxgiFactory), this->dxgiFactory);
		if (FAILED(hres))
		{
			return ResultError(Failure, std::string("GrafSystemDX12: CreateDXGIFactory2 failed with HRESULT = ") + HResultToString(hres));
		}

		// enumerate physical devices

		std::vector<shared_ref<IDXGIAdapter1>> dxgiAdapters;
		shared_ref<IDXGIAdapter1> dxgiAdapter;
		ur_size adapterCount = 0;
		while (this->dxgiFactory->EnumAdapters1((ur_uint)adapterCount, dxgiAdapter) != DXGI_ERROR_NOT_FOUND)
		{
			dxgiAdapters.push_back(dxgiAdapter);
			dxgiAdapter.reset();
			++adapterCount;
		}
		this->grafPhysicalDeviceDesc.resize(adapterCount);

		for (ur_size iadapter = 0; iadapter < adapterCount; ++iadapter)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
			hres = dxgiAdapters[iadapter]->GetDesc1(&dxgiAdapterDesc);
			if (FAILED(hres))
				break;

			GrafPhysicalDeviceDesc& grafDeviceDesc = this->grafPhysicalDeviceDesc[iadapter];
			grafDeviceDesc = {};
			grafDeviceDesc.Description = _bstr_t(dxgiAdapterDesc.Description);
			grafDeviceDesc.VendorId = (ur_uint)dxgiAdapterDesc.VendorId;
			grafDeviceDesc.DeviceId = (ur_uint)dxgiAdapterDesc.DeviceId;
			grafDeviceDesc.DedicatedVideoMemory = (ur_size)dxgiAdapterDesc.DedicatedVideoMemory;
			grafDeviceDesc.SharedSystemMemory = (ur_size)dxgiAdapterDesc.SharedSystemMemory;
			grafDeviceDesc.DedicatedSystemMemory = (ur_size)dxgiAdapterDesc.DedicatedSystemMemory;
			grafDeviceDesc.ConstantBufferOffsetAlignment = 256;
		}
		if (FAILED(hres))
		{
			this->Deinitialize();
			return ResultError(Failure, std::string("GrafSystemDX12: failed to enumerate adapters with HRESULT = ") + HResultToString(hres));
		}

		return Result(NotImplemented);
	}

	Result GrafSystemDX12::CreateDevice(std::unique_ptr<GrafDevice>& grafDevice)
	{
		return Result(NotImplemented);
	}

	Result GrafSystemDX12::CreateCommandList(std::unique_ptr<GrafCommandList>& grafCommandList)
	{
		return Result(NotImplemented);
	}

	Result GrafSystemDX12::CreateFence(std::unique_ptr<GrafFence>& grafFence)
	{
		return Result(NotImplemented);
	}

	Result GrafSystemDX12::CreateCanvas(std::unique_ptr<GrafCanvas>& grafCanvas)
	{
		return Result(NotImplemented);
	}

	Result GrafSystemDX12::CreateImage(std::unique_ptr<GrafImage>& grafImage)
	{
		return Result(NotImplemented);
	}

	Result GrafSystemDX12::CreateBuffer(std::unique_ptr<GrafBuffer>& grafBuffer)
	{
		return Result(NotImplemented);
	}

	Result GrafSystemDX12::CreateSampler(std::unique_ptr<GrafSampler>& grafSampler)
	{
		return Result(NotImplemented);
	}

	Result GrafSystemDX12::CreateShader(std::unique_ptr<GrafShader>& grafShader)
	{
		return Result(NotImplemented);
	}

	Result GrafSystemDX12::CreateRenderPass(std::unique_ptr<GrafRenderPass>& grafRenderPass)
	{
		return Result(NotImplemented);
	}

	Result GrafSystemDX12::CreateRenderTarget(std::unique_ptr<GrafRenderTarget>& grafRenderTarget)
	{
		return Result(NotImplemented);
	}

	Result GrafSystemDX12::CreateDescriptorTableLayout(std::unique_ptr<GrafDescriptorTableLayout>& grafDescriptorTableLayout)
	{
		return Result(NotImplemented);
	}

	Result GrafSystemDX12::CreateDescriptorTable(std::unique_ptr<GrafDescriptorTable>& grafDescriptorTable)
	{
		return Result(NotImplemented);
	}

	Result GrafSystemDX12::CreatePipeline(std::unique_ptr<GrafPipeline>& grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafSystemDX12::CreateRayTracingPipeline(std::unique_ptr<GrafRayTracingPipeline>& grafRayTracingPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafSystemDX12::CreateAccelerationStructure(std::unique_ptr<GrafAccelerationStructure>& grafAccelStruct)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafDeviceDX12::GrafDeviceDX12(GrafSystem &grafSystem) :
		GrafDevice(grafSystem)
	{
	}

	GrafDeviceDX12::~GrafDeviceDX12()
	{
		this->Deinitialize();
	}

	Result GrafDeviceDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafDeviceDX12::Initialize(ur_uint deviceId)
	{
		return Result(NotImplemented);
	}

	Result GrafDeviceDX12::Record(GrafCommandList* grafCommandList)
	{
		return Result(NotImplemented);
	}

	Result GrafDeviceDX12::Submit()
	{
		return Result(NotImplemented);
	}

	Result GrafDeviceDX12::WaitIdle()
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafCommandListDX12::GrafCommandListDX12(GrafSystem &grafSystem) :
		GrafCommandList(grafSystem)
	{
	}

	GrafCommandListDX12::~GrafCommandListDX12()
	{
		this->Deinitialize();
	}

	Result GrafCommandListDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::Initialize(GrafDevice *grafDevice)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::Begin()
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::End()
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::Wait(ur_uint64 timeout)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::ImageMemoryBarrier(GrafImage* grafImage, GrafImageState srcState, GrafImageState dstState)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BufferMemoryBarrier(GrafBuffer* grafBuffer, GrafBufferState srcState, GrafBufferState dstState)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::SetFenceState(GrafFence* grafFence, GrafFenceState state)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::ClearColorImage(GrafImage* grafImage, GrafClearValue clearValue)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::SetViewport(const GrafViewportDesc& grafViewportDesc, ur_bool resetScissorsRect)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::SetScissorsRect(const RectI& scissorsRect)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BeginRenderPass(GrafRenderPass* grafRenderPass, GrafRenderTarget* grafRenderTarget, GrafClearValue* rtClearValues)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::EndRenderPass()
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BindPipeline(GrafPipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BindDescriptorTable(GrafDescriptorTable* descriptorTable, GrafPipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BindVertexBuffer(GrafBuffer* grafVertexBuffer, ur_uint bindingIdx, ur_size bufferOffset)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BindIndexBuffer(GrafBuffer* grafIndexBuffer, GrafIndexType indexType, ur_size bufferOffset)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::Draw(ur_uint vertexCount, ur_uint instanceCount, ur_uint firstVertex, ur_uint firstInstance)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::DrawIndexed(ur_uint indexCount, ur_uint instanceCount, ur_uint firstIndex, ur_uint firstVertex, ur_uint firstInstance)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::Copy(GrafBuffer* srcBuffer, GrafBuffer* dstBuffer, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::Copy(GrafBuffer* srcBuffer, GrafImage* dstImage, ur_size bufferOffset, BoxI imageRegion)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::Copy(GrafImage* srcImage, GrafBuffer* dstBuffer, ur_size bufferOffset, BoxI imageRegion)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::Copy(GrafImage* srcImage, GrafImage* dstImage, BoxI srcRegion, BoxI dstRegion)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BindComputePipeline(GrafPipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BindComputeDescriptorTable(GrafDescriptorTable* descriptorTable, GrafPipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::Dispatch(ur_uint groupCountX, ur_uint groupCountY, ur_uint groupCountZ)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BuildAccelerationStructure(GrafAccelerationStructure* dstStructrure, GrafAccelerationStructureGeometryData* geometryData, ur_uint geometryCount)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BindRayTracingPipeline(GrafRayTracingPipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::BindRayTracingDescriptorTable(GrafDescriptorTable* descriptorTable, GrafRayTracingPipeline* grafPipeline)
	{
		return Result(NotImplemented);
	}

	Result GrafCommandListDX12::DispatchRays(ur_uint width, ur_uint height, ur_uint depth,
		const GrafStridedBufferRegionDesc* rayGenShaderTable, const GrafStridedBufferRegionDesc* missShaderTable,
		const GrafStridedBufferRegionDesc* hitShaderTable, const GrafStridedBufferRegionDesc* callableShaderTable)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafFenceDX12::GrafFenceDX12(GrafSystem &grafSystem) :
		GrafFence(grafSystem)
	{
	}

	GrafFenceDX12::~GrafFenceDX12()
	{
		this->Deinitialize();
	}

	Result GrafFenceDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafFenceDX12::Initialize(GrafDevice *grafDevice)
	{
		GrafDeviceEntity::Initialize(grafDevice);
		return Result(NotImplemented);
	}

	Result GrafFenceDX12::SetState(GrafFenceState state)
	{
		return Result(NotImplemented);
	}

	Result GrafFenceDX12::GetState(GrafFenceState& state)
	{
		return Result(NotImplemented);
	}

	Result GrafFenceDX12::WaitSignaled()
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafCanvasDX12::GrafCanvasDX12(GrafSystem &grafSystem) :
		GrafCanvas(grafSystem)
	{
		this->frameCount = 0;
		this->frameIdx = 0;
	}

	GrafCanvasDX12::~GrafCanvasDX12()
	{
		this->Deinitialize();
	}

	Result GrafCanvasDX12::Deinitialize()
	{
		this->frameCount = 0;
		this->frameIdx = 0;
		this->swapChainImageCount = 0;
		this->swapChainCurrentImageId = 0;

		return Result(NotImplemented);
	}

	Result GrafCanvasDX12::Initialize(GrafDevice* grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	Result GrafCanvasDX12::AcquireNextImage()
	{
		return Result(NotImplemented);
	}

	Result GrafCanvasDX12::Present()
	{
		return Result(NotImplemented);
	}

	GrafImage* GrafCanvasDX12::GetCurrentImage()
	{
		return ur_null;
	}

	GrafImage* GrafCanvasDX12::GetSwapChainImage(ur_uint imageId)
	{
		return ur_null;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafImageDX12::GrafImageDX12(GrafSystem &grafSystem) :
		GrafImage(grafSystem)
	{
	}

	GrafImageDX12::~GrafImageDX12()
	{
		this->Deinitialize();
	}

	Result GrafImageDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafImageDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	Result GrafImageDX12::InitializeFromD3DResource(GrafDevice *grafDevice, const InitParams& initParams, ID3D12Resource* d3dResource)
	{
		return Result(NotImplemented);
	}

	Result GrafImageDX12::Write(const ur_byte* dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	Result GrafImageDX12::Write(GrafWriteCallback writeCallback, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	Result GrafImageDX12::Read(ur_byte*& dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafBufferDX12::GrafBufferDX12(GrafSystem &grafSystem) :
		GrafBuffer(grafSystem)
	{
	}

	GrafBufferDX12::~GrafBufferDX12()
	{
		this->Deinitialize();
	}

	Result GrafBufferDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafBufferDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	Result GrafBufferDX12::Write(const ur_byte* dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	Result GrafBufferDX12::Write(GrafWriteCallback writeCallback, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	Result GrafBufferDX12::Read(ur_byte*& dataPtr, ur_size dataSize, ur_size srcOffset, ur_size dstOffset)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafSamplerDX12::GrafSamplerDX12(GrafSystem &grafSystem) :
		GrafSampler(grafSystem)
	{
	}

	GrafSamplerDX12::~GrafSamplerDX12()
	{
		this->Deinitialize();
	}

	Result GrafSamplerDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafSamplerDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafShaderDX12::GrafShaderDX12(GrafSystem &grafSystem) :
		GrafShader(grafSystem)
	{
	}

	GrafShaderDX12::~GrafShaderDX12()
	{
		this->Deinitialize();
	}

	Result GrafShaderDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafShaderDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafRenderTargetDX12::GrafRenderTargetDX12(GrafSystem &grafSystem) :
		GrafRenderTarget(grafSystem)
	{
	}

	GrafRenderTargetDX12::~GrafRenderTargetDX12()
	{
		this->Deinitialize();
	}

	Result GrafRenderTargetDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafRenderTargetDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafDescriptorTableLayoutDX12::GrafDescriptorTableLayoutDX12(GrafSystem &grafSystem) :
		GrafDescriptorTableLayout(grafSystem)
	{
	}

	GrafDescriptorTableLayoutDX12::~GrafDescriptorTableLayoutDX12()
	{
		this->Deinitialize();
	}

	Result GrafDescriptorTableLayoutDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableLayoutDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafDescriptorTableDX12::GrafDescriptorTableDX12(GrafSystem &grafSystem) :
		GrafDescriptorTable(grafSystem)
	{
	}

	GrafDescriptorTableDX12::~GrafDescriptorTableDX12()
	{
		this->Deinitialize();
	}

	Result GrafDescriptorTableDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetConstantBuffer(ur_uint bindingIdx, GrafBuffer* buffer, ur_size bufferOfs, ur_size bufferRange)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetSampledImage(ur_uint bindingIdx, GrafImage* image, GrafSampler* sampler)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetSampler(ur_uint bindingIdx, GrafSampler* sampler)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetImage(ur_uint bindingIdx, GrafImage* image)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetBuffer(ur_uint bindingIdx, GrafBuffer* buffer)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetRWImage(ur_uint bindingIdx, GrafImage* image)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetRWBuffer(ur_uint bindingIdx, GrafBuffer* buffer)
	{
		return Result(NotImplemented);
	}

	Result GrafDescriptorTableDX12::SetAccelerationStructure(ur_uint bindingIdx, GrafAccelerationStructure* accelerationStructure)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafPipelineDX12::GrafPipelineDX12(GrafSystem &grafSystem) :
		GrafPipeline(grafSystem)
	{
	}

	GrafPipelineDX12::~GrafPipelineDX12()
	{
		this->Deinitialize();
	}

	Result GrafPipelineDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafPipelineDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafRayTracingPipelineDX12::GrafRayTracingPipelineDX12(GrafSystem &grafSystem) :
		GrafRayTracingPipeline(grafSystem)
	{
	}

	GrafRayTracingPipelineDX12::~GrafRayTracingPipelineDX12()
	{
		this->Deinitialize();
	}

	Result GrafRayTracingPipelineDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafRayTracingPipelineDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	Result GrafRayTracingPipelineDX12::GetShaderGroupHandles(ur_uint firstGroup, ur_uint groupCount, ur_size dstBufferSize, ur_byte* dstBufferPtr)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafRenderPassDX12::GrafRenderPassDX12(GrafSystem& grafSystem) :
		GrafRenderPass(grafSystem)
	{
	}

	GrafRenderPassDX12::~GrafRenderPassDX12()
	{
		this->Deinitialize();
	}

	Result GrafRenderPassDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafRenderPassDX12::Initialize(GrafDevice* grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GrafAccelerationStructureDX12::GrafAccelerationStructureDX12(GrafSystem &grafSystem) :
		GrafAccelerationStructure(grafSystem)
	{
	}

	GrafAccelerationStructureDX12::~GrafAccelerationStructureDX12()
	{
		this->Deinitialize();
	}

	Result GrafAccelerationStructureDX12::Deinitialize()
	{
		return Result(NotImplemented);
	}

	Result GrafAccelerationStructureDX12::Initialize(GrafDevice *grafDevice, const InitParams& initParams)
	{
		return Result(NotImplemented);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GrafUtilsDX12

} // end namespace UnlimRealms