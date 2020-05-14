///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GRAF: DIRECTX12 IMPLEMENTATION
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Graf/GrafSystem.h"
#include "Sys/Windows/WinUtils.h"
#include <dxgi1_4.h>
#include <d3d12.h>

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafSystemDX12 : public GrafSystem
	{
	public:

		GrafSystemDX12(Realm &realm);

		virtual ~GrafSystemDX12();

		virtual Result Initialize(Canvas *canvas);

		virtual Result CreateDevice(std::unique_ptr<GrafDevice>& grafDevice);

		virtual Result CreateCommandList(std::unique_ptr<GrafCommandList>& grafCommandList);

		virtual Result CreateFence(std::unique_ptr<GrafFence>& grafFence);

		virtual Result CreateCanvas(std::unique_ptr<GrafCanvas>& grafCanvas);

		virtual Result CreateImage(std::unique_ptr<GrafImage>& grafImage);

		virtual Result CreateBuffer(std::unique_ptr<GrafBuffer>& grafBuffer);

		virtual Result CreateSampler(std::unique_ptr<GrafSampler>& grafSampler);

		virtual Result CreateShader(std::unique_ptr<GrafShader>& grafShader);

		virtual Result CreateRenderPass(std::unique_ptr<GrafRenderPass>& grafRenderPass);

		virtual Result CreateRenderTarget(std::unique_ptr<GrafRenderTarget>& grafRenderTarget);

		virtual Result CreateDescriptorTableLayout(std::unique_ptr<GrafDescriptorTableLayout>& grafDescriptorTableLayout);

		virtual Result CreateDescriptorTable(std::unique_ptr<GrafDescriptorTable>& grafDescriptorTable);

		virtual Result CreatePipeline(std::unique_ptr<GrafPipeline>& grafPipeline);

		virtual Result CreateRayTracingPipeline(std::unique_ptr<GrafRayTracingPipeline>& grafRayTracingPipeline);

		virtual Result CreateAccelerationStructure(std::unique_ptr<GrafAccelerationStructure>& grafAccelStruct);

	private:

		Result Deinitialize();

		shared_ref<IDXGIFactory4> dxgiFactory;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafDeviceDX12 : public GrafDevice
	{
	public:

		GrafDeviceDX12(GrafSystem &grafSystem);

		~GrafDeviceDX12();

		virtual Result Initialize(ur_uint deviceId);

		virtual Result Record(GrafCommandList* grafCommandList);

		virtual Result Submit();

		virtual Result WaitIdle();

	private:

		Result Deinitialize();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafCommandListDX12 : public GrafCommandList
	{
	public:

		GrafCommandListDX12(GrafSystem &grafSystem);

		~GrafCommandListDX12();

		virtual Result Initialize(GrafDevice *grafDevice);

		virtual Result Begin();

		virtual Result End();

		virtual Result Wait(ur_uint64 timeout = ur_uint64(-1));

		virtual Result BufferMemoryBarrier(GrafBuffer* grafBuffer, GrafBufferState srcState, GrafBufferState dstState);

		virtual Result ImageMemoryBarrier(GrafImage* grafImage, GrafImageState srcState, GrafImageState dstState);

		virtual Result SetFenceState(GrafFence* grafFence, GrafFenceState state);

		virtual Result ClearColorImage(GrafImage* grafImage, GrafClearValue clearValue);

		virtual Result SetViewport(const GrafViewportDesc& grafViewportDesc, ur_bool resetScissorsRect = false);

		virtual Result SetScissorsRect(const RectI& scissorsRect);

		virtual Result BeginRenderPass(GrafRenderPass* grafRenderPass, GrafRenderTarget* grafRenderTarget, GrafClearValue* rtClearValues = ur_null);

		virtual Result EndRenderPass();

		virtual Result BindPipeline(GrafPipeline* grafPipeline);

		virtual Result BindDescriptorTable(GrafDescriptorTable* descriptorTable, GrafPipeline* grafPipeline);

		virtual Result BindVertexBuffer(GrafBuffer* grafVertexBuffer, ur_uint bindingIdx, ur_size bufferOffset = 0);

		virtual Result BindIndexBuffer(GrafBuffer* grafIndexBuffer, GrafIndexType indexType, ur_size bufferOffset = 0);

		virtual Result Draw(ur_uint vertexCount, ur_uint instanceCount, ur_uint firstVertex, ur_uint firstInstance);

		virtual Result DrawIndexed(ur_uint indexCount, ur_uint instanceCount, ur_uint firstIndex, ur_uint firstVertex, ur_uint firstInstance);

		virtual Result Copy(GrafBuffer* srcBuffer, GrafBuffer* dstBuffer, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Copy(GrafBuffer* srcBuffer, GrafImage* dstImage, ur_size bufferOffset = 0, BoxI imageRegion = BoxI::Zero);

		virtual Result Copy(GrafImage* srcImage, GrafBuffer* dstBuffer, ur_size bufferOffset = 0, BoxI imageRegion = BoxI::Zero);

		virtual Result Copy(GrafImage* srcImage, GrafImage* dstImage, BoxI srcRegion = BoxI::Zero, BoxI dstRegion = BoxI::Zero);

		virtual Result BindComputePipeline(GrafPipeline* grafPipeline);

		virtual Result BindComputeDescriptorTable(GrafDescriptorTable* descriptorTable, GrafPipeline* grafPipeline);

		virtual Result Dispatch(ur_uint groupCountX, ur_uint groupCountY, ur_uint groupCountZ);

		virtual Result BuildAccelerationStructure(GrafAccelerationStructure* dstStructrure, GrafAccelerationStructureGeometryData* geometryData, ur_uint geometryCount);

		virtual Result BindRayTracingPipeline(GrafRayTracingPipeline* grafPipeline);

		virtual Result BindRayTracingDescriptorTable(GrafDescriptorTable* descriptorTable, GrafRayTracingPipeline* grafPipeline);

		virtual Result DispatchRays(ur_uint width, ur_uint height, ur_uint depth,
			const GrafStridedBufferRegionDesc* rayGenShaderTable, const GrafStridedBufferRegionDesc* missShaderTable,
			const GrafStridedBufferRegionDesc* hitShaderTable, const GrafStridedBufferRegionDesc* callableShaderTable);

	private:

		Result Deinitialize();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafFenceDX12 : public GrafFence
	{
	public:

		GrafFenceDX12(GrafSystem &grafSystem);

		~GrafFenceDX12();

		virtual Result Initialize(GrafDevice *grafDevice);

		virtual Result SetState(GrafFenceState state);

		virtual Result GetState(GrafFenceState& state);

		virtual Result WaitSignaled();

	private:

		Result Deinitialize();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafCanvasDX12 : public GrafCanvas
	{
	public:

		GrafCanvasDX12(GrafSystem &grafSystem);

		~GrafCanvasDX12();

		virtual Result Initialize(GrafDevice* grafDevice, const InitParams& initParams);

		virtual Result Present();

		virtual GrafImage* GetCurrentImage();

		virtual GrafImage* GetSwapChainImage(ur_uint imageId);

	private:

		Result Deinitialize();

		Result AcquireNextImage();

		std::vector<std::unique_ptr<GrafImage>> swapChainImages;

		// per frame data
		ur_uint32 frameCount;
		ur_uint32 frameIdx;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafImageDX12 : public GrafImage
	{
	public:

		GrafImageDX12(GrafSystem &grafSystem);

		~GrafImageDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result Write(const ur_byte* dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Write(GrafWriteCallback writeCallback, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Read(ur_byte*& dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		Result InitializeFromD3DResource(GrafDevice *grafDevice, const InitParams& initParams, ID3D12Resource* d3dResource);

	private:

		Result Deinitialize();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafBufferDX12 : public GrafBuffer
	{
	public:

		GrafBufferDX12(GrafSystem &grafSystem);

		~GrafBufferDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result Write(const ur_byte* dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Write(GrafWriteCallback writeCallback, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

		virtual Result Read(ur_byte*& dataPtr, ur_size dataSize = 0, ur_size srcOffset = 0, ur_size dstOffset = 0);

	private:

		Result Deinitialize();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafSamplerDX12 : public GrafSampler
	{
	public:

		GrafSamplerDX12(GrafSystem &grafSystem);

		~GrafSamplerDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

	protected:

		Result Deinitialize();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafShaderDX12 : public GrafShader
	{
	public:

		GrafShaderDX12(GrafSystem &grafSystem);

		~GrafShaderDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

	private:

		Result Deinitialize();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafRenderPassDX12 : public GrafRenderPass
	{
	public:

		GrafRenderPassDX12(GrafSystem& grafSystem);

		~GrafRenderPassDX12();

		virtual Result Initialize(GrafDevice* grafDevice, const InitParams& initParams);

	private:

		Result Deinitialize();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafRenderTargetDX12 : public GrafRenderTarget
	{
	public:

		GrafRenderTargetDX12(GrafSystem &grafSystem);

		~GrafRenderTargetDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

	private:

		Result Deinitialize();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafDescriptorTableLayoutDX12 : public GrafDescriptorTableLayout
	{
	public:

		GrafDescriptorTableLayoutDX12(GrafSystem &grafSystem);

		~GrafDescriptorTableLayoutDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

	private:

		Result Deinitialize();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafDescriptorTableDX12 : public GrafDescriptorTable
	{
	public:

		GrafDescriptorTableDX12(GrafSystem &grafSystem);

		~GrafDescriptorTableDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result SetConstantBuffer(ur_uint bindingIdx, GrafBuffer* buffer, ur_size bufferOfs = 0, ur_size bufferRange = 0);

		virtual Result SetSampledImage(ur_uint bindingIdx, GrafImage* image, GrafSampler* sampler);

		virtual Result SetSampler(ur_uint bindingIdx, GrafSampler* sampler);

		virtual Result SetImage(ur_uint bindingIdx, GrafImage* image);

		virtual Result SetBuffer(ur_uint bindingIdx, GrafBuffer* buffer);

		virtual Result SetRWImage(ur_uint bindingIdx, GrafImage* image);

		virtual Result SetRWBuffer(ur_uint bindingIdx, GrafBuffer* buffer);

		virtual Result SetAccelerationStructure(ur_uint bindingIdx, GrafAccelerationStructure* accelerationStructure);

	private:

		Result Deinitialize();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafPipelineDX12 : public GrafPipeline
	{
	public:

		GrafPipelineDX12(GrafSystem &grafSystem);

		~GrafPipelineDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

	protected:

		Result Deinitialize();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafRayTracingPipelineDX12 : public GrafRayTracingPipeline
	{
	public:

		GrafRayTracingPipelineDX12(GrafSystem &grafSystem);

		~GrafRayTracingPipelineDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		virtual Result GetShaderGroupHandles(ur_uint firstGroup, ur_uint groupCount, ur_size dstBufferSize, ur_byte* dstBufferPtr);

	protected:

		Result Deinitialize();
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafAccelerationStructureDX12 : public GrafAccelerationStructure
	{
	public:

		GrafAccelerationStructureDX12(GrafSystem &grafSystem);

		~GrafAccelerationStructureDX12();

		virtual Result Initialize(GrafDevice *grafDevice, const InitParams& initParams);

		inline GrafBuffer* GetScratchBuffer() const;

	protected:

		Result Deinitialize();

		std::unique_ptr<GrafBuffer> grafScratchBuffer;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL GrafUtilsDX12
	{
	public:

		
	};

} // end namespace UnlimRealms

#include "GrafSystemDX12.inline.h"