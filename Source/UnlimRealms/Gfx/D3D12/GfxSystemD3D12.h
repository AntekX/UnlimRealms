///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Gfx/GfxSystem.h"
#include "Sys/Windows/WinUtils.h"

// DirectX

#include <dxgi1_4.h>
#include <d3d12.h>
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

namespace UnlimRealms
{

	// forward declarations
	class GfxSystemD3D12;
	class GfxContextD3D12;
	class GfxResourceD3D12;
	class GfxTextureD3D12;
	class GfxRenderTargetD3D12;
	class GfxSwapChainD3D12;
	class GfxBufferD3D12;
	class GfxVertexShaderD3D12;
	class GfxPixelShaderD3D12;
	class GfxInputLayoutD3D12;
	class GfxPipelineStateObjectD3D12;

	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics system
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxSystemD3D12 : public GfxSystem
	{
	public:

		GfxSystemD3D12(Realm &realm);

		virtual ~GfxSystemD3D12();

		virtual Result Initialize(Canvas *canvas);

		virtual Result Render();

		virtual Result CreateContext(std::unique_ptr<GfxContext> &gfxContext);

		virtual Result CreateTexture(std::unique_ptr<GfxTexture> &gfxTexture);

		virtual Result CreateRenderTarget(std::unique_ptr<GfxRenderTarget> &gfxRT);

		virtual Result CreateSwapChain(std::unique_ptr<GfxSwapChain> &gfxSwapChain);

		virtual Result CreateBuffer(std::unique_ptr<GfxBuffer> &gfxBuffer);
		
		virtual Result CreateResourceBinding(std::unique_ptr<GfxResourceBinding> &gfxBinding);

		virtual Result CreatePipelineStateObject(std::unique_ptr<GfxPipelineStateObject> &gfxPipelineState);

		Result InitializeFrameData(ur_uint framesCount);

		Result SetFrame(ur_uint frameIndex);

		Result SetNextFrame();

		Result WaitFrame(ur_uint frameIndex);

		Result WaitCurrentFrame();

		Result WaitGPU();

		Result AddCommandList(shared_ref<ID3D12CommandList> &d3dCommandList);		

		inline ur_bool IsFrameComplete(ur_uint frameIndex);

		inline ur_bool IsCurrentFrameComplete();

		inline ur_uint CurrentFrameIndex() const;

		inline WinCanvas* GetWinCanvas() const;

		inline IDXGIFactory4* GetDXGIFactory() const;

		inline ID3D12Device* GetD3DDevice() const;
		
		inline ID3D12CommandQueue* GetD3DCommandQueue() const;

		inline ID3D12CommandAllocator* GetD3DCommandAllocator() const;


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Descriptor & Descriptors Heap
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		class UR_DECL DescriptorHeap;
		class UR_DECL DescriptorSet;
		class UR_DECL Descriptor;

		class UR_DECL DescriptorHeap : public GfxEntity
		{
		public:

			DescriptorHeap(GfxSystemD3D12& gfxSystem, D3D12_DESCRIPTOR_HEAP_TYPE d3dHeapType);

			~DescriptorHeap();

			Result AcquireDescriptor(std::unique_ptr<Descriptor>& descriptor, ur_bool shaderVisible = false);

			Result AcquireDescriptors(std::unique_ptr<DescriptorSet>& descriptorSet, ur_size descriptorsCount, ur_bool shaderVisible = false);

			Result ReleaseDescriptor(Descriptor& descriptor);

			Result ReleaseDescriptors(DescriptorSet& descriptorSet);

			inline ID3D12DescriptorHeap* GetD3DDescriptorHeap(DescriptorSet& descriptorSet);

		private:

			Result AcquireDescriptors(DescriptorSet& descriptorSet, ur_size descriptorsCount, ur_bool shaderVisible = false);

			static const ur_size DescriptorsPerHeap = 256;

			struct Range
			{
				ur_size start;
				ur_size end;
				Range() : start(0), end(0) {}
				Range(ur_size start, ur_size end) : start(start), end(end) {}
				inline ur_size size() const { return end - start; }
			};

			struct Page : public NonCopyable
			{
				shared_ref<ID3D12DescriptorHeap> d3dHeap;
				std::vector<Range> freeRanges; // todo: optimize ranges
			};

			D3D12_DESCRIPTOR_HEAP_TYPE d3dHeapType;
			std::mutex modifyMutex;
			std::vector<std::unique_ptr<Page>> pagePool[2]; // 0 - cpu read/write; 1 - cpu write, gpu read heaps;
			std::vector<DescriptorSet*> descriptorSets;
			ur_uint32 d3dDescriptorSize;
			ur_size firstFreePageIdx;
		};

		class UR_DECL DescriptorSet
		{
		public:

			DescriptorSet();

			~DescriptorSet();

			inline ur_size GetDescriptorCount() const;

			inline D3D12_CPU_DESCRIPTOR_HANDLE FirstCpuHandle() const;

			inline D3D12_GPU_DESCRIPTOR_HANDLE FirstGpuHandle() const;

		private:

			friend class DescriptorHeap;
			ur_size descriptorCount;
			ur_bool shaderVisible;
			D3D12_CPU_DESCRIPTOR_HANDLE firstCpuHandle;
			D3D12_GPU_DESCRIPTOR_HANDLE firstGpuHandle;
			DescriptorHeap* heap;
			ur_size pagePoolPos;
			ur_size descriptorListPos;
		};

		class UR_DECL Descriptor : public DescriptorSet
		{
		public:

			Descriptor();

			~Descriptor();

			inline D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle() const;

			inline D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle() const;
		};

		inline DescriptorHeap* GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType);

		
	private:

		Result InitializeDXGI();

		Result InitializeDevice(IDXGIAdapter *pAdapter, const D3D_FEATURE_LEVEL minimalFeatureLevel);

		void ReleaseDXGIObjects();

		void ReleaseDeviceObjects();

		WinCanvas *winCanvas;
		shared_ref<IDXGIFactory4> dxgiFactory;
		std::vector<shared_ref<IDXGIAdapter1>> dxgiAdapters;
		shared_ref<ID3D12Device> d3dDevice;
		shared_ref<ID3D12CommandQueue> d3dCommandQueue;
		std::vector<shared_ref<ID3D12CommandList>> d3dCommandLists;
		std::mutex commandListsMutex;
		std::vector<std::unique_ptr<DescriptorHeap>> descriptorHeaps;
		std::vector<shared_ref<ID3D12CommandAllocator>> d3dCommandAllocators;
		std::vector<ur_uint> frameFenceValues;
		shared_ref<ID3D12Fence> d3dFrameFence;
		HANDLE frameFenceEvent;
		ur_uint frameIndex;
		ur_uint framesCount;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics context
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxContextD3D12 : public GfxContext
	{
	public:

		GfxContextD3D12(GfxSystem &gfxSystem);

		virtual ~GfxContextD3D12();

		virtual Result Initialize();

		virtual Result Begin();

		virtual Result End();

		virtual Result SetRenderTarget(GfxRenderTarget *rt);

		virtual Result SetRenderTarget(GfxRenderTarget *rt, GfxRenderTarget *ds);

		virtual Result SetViewPort(const GfxViewPort *viewPort);

		virtual Result GetViewPort(GfxViewPort &viewPort);

		virtual Result SetScissorRect(const RectI *rect);

		virtual Result ClearTarget(GfxRenderTarget *rt,
			bool clearColor, const ur_float4 &color,
			bool clearDepth, const ur_float &depth,
			bool clearStencil, const ur_uint &stencil);

		virtual Result SetPipelineStateObject(GfxPipelineStateObject *state);

		virtual Result SetResourceBinding(GfxResourceBinding *binding);

		virtual Result SetVertexBuffer(GfxBuffer *buffer, ur_uint slot);

		virtual Result SetIndexBuffer(GfxBuffer *buffer);

		virtual Result Draw(ur_uint vertexCount, ur_uint vertexOffset, ur_uint instanceCount, ur_uint instanceOffset);

		virtual Result DrawIndexed(ur_uint indexCount, ur_uint indexOffset, ur_uint vertexOffset, ur_uint instanceCount, ur_uint instanceOffset);

		virtual Result UpdateBuffer(GfxBuffer *buffer, GfxGPUAccess gpuAccess, bool doNotWait, UpdateBufferCallback callback);

		
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// D3D12 specific functions

		Result UpdateResource(GfxResourceD3D12* dstResource, GfxResourceD3D12* srcResource);

		Result ResourceTransition(GfxResourceD3D12 *resource, D3D12_RESOURCE_STATES newState);

		inline ID3D12GraphicsCommandList* GetD3DCommandList() const;

	private:

		shared_ref<ID3D12GraphicsCommandList> d3dCommandList;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics resource
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxResourceD3D12 : public GfxEntity
	{
	public:

		GfxResourceD3D12(GfxSystem &gfxSystem);

		virtual ~GfxResourceD3D12();

		Result Initialize(ID3D12Resource* d3dResource, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATES(-1));

		Result Initialize(GfxResourceD3D12& resource);

		inline ID3D12Resource* GetD3DResource() const;

		inline D3D12_RESOURCE_STATES GetD3DResourceState();

	private:

		friend class GfxContextD3D12;

		shared_ref<ID3D12Resource> d3dResource;
		D3D12_RESOURCE_STATES d3dCurrentState;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics texture
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxTextureD3D12 : public GfxTexture
	{
	public:

		GfxTextureD3D12(GfxSystem &gfxSystem);

		virtual ~GfxTextureD3D12();

		Result Initialize(const GfxTextureDesc &desc, GfxResourceD3D12 &resource);

		inline GfxResourceD3D12& GetResource();

		inline GfxSystemD3D12::Descriptor* GetSRVDescriptor() const;

	protected:

		virtual Result OnInitialize(const GfxResourceData *data);

	private:

		bool initializedFromD3DRes;
		GfxResourceD3D12 resource;
		std::unique_ptr<GfxSystemD3D12::Descriptor> srvDescriptor;
		std::unique_ptr<GfxResourceD3D12> uploadResource;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics render target
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxRenderTargetD3D12 : public GfxRenderTarget
	{
	public:

		GfxRenderTargetD3D12(GfxSystem &gfxSystem);

		virtual ~GfxRenderTargetD3D12();

		inline GfxSystemD3D12::Descriptor* GetRTVDescriptor() const;
		
		inline GfxSystemD3D12::Descriptor* GetDSVDescriptor() const;

	protected:

		virtual Result OnInitialize();

	private:

		std::unique_ptr<GfxSystemD3D12::Descriptor> rtvDescriptor;
		std::unique_ptr<GfxSystemD3D12::Descriptor> dsvDescriptor;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 render targets swap chain
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxSwapChainD3D12 : public GfxSwapChain
	{
	public:

		GfxSwapChainD3D12(GfxSystem &gfxSystem);

		virtual ~GfxSwapChainD3D12();

		virtual Result Initialize(const GfxPresentParams &params);

		virtual Result Present();

		virtual GfxRenderTarget* GetTargetBuffer();

		inline IDXGISwapChain3* GetDXGISwapChain() const;

	protected:

		class UR_DECL BackBuffer : public GfxRenderTargetD3D12
		{
		public:

			BackBuffer(GfxSystem &gfxSystem, shared_ref<ID3D12Resource> d3dSwapChainResource);

			virtual ~BackBuffer();

		protected:

			virtual Result InitializeTargetBuffer(std::unique_ptr<GfxTexture> &resultBuffer, const GfxTextureDesc &desc);

		private:

			GfxResourceD3D12 dxgiSwapChainBuffer;
		};

	private:

		DXGI_SWAP_CHAIN_DESC1 dxgiChainDesc;
		shared_ref<IDXGISwapChain3> dxgiSwapChain;
		ur_uint backBufferIndex;
		std::vector<std::unique_ptr<BackBuffer>> backBuffers;
		GfxContextD3D12 gfxContext;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics buffer
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxBufferD3D12 : public GfxBuffer
	{
	public:

		GfxBufferD3D12(GfxSystem &gfxSystem);

		virtual ~GfxBufferD3D12();

		inline GfxResourceD3D12& GetResource();

		inline const D3D12_VERTEX_BUFFER_VIEW& GetD3DViewVB() const;

		inline const D3D12_INDEX_BUFFER_VIEW& GetD3DViewIB() const;

		inline const D3D12_CONSTANT_BUFFER_VIEW_DESC& GetD3DViewCB() const;

		inline GfxSystemD3D12::Descriptor* GetDescriptor() const;

	protected:

		virtual Result OnInitialize(const GfxResourceData *data);

	private:

		GfxResourceD3D12 resource;
		std::unique_ptr<GfxResourceD3D12> uploadResource;
		union {
			D3D12_VERTEX_BUFFER_VIEW d3dVBView;
			D3D12_INDEX_BUFFER_VIEW d3dIBView;
			D3D12_CONSTANT_BUFFER_VIEW_DESC d3dCBView;
		};
		std::unique_ptr<GfxSystemD3D12::Descriptor> descriptor;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics pipeline state object
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxPipelineStateObjectD3D12 : public GfxPipelineStateObject
	{
	public:

		GfxPipelineStateObjectD3D12(GfxSystem &gfxSystem);

		virtual ~GfxPipelineStateObjectD3D12();

		inline ID3D12PipelineState* GetD3DPipelineState() const;

		inline D3D12_PRIMITIVE_TOPOLOGY GetD3DPrimitiveTopology() const;

	protected:

		virtual Result OnInitialize(const StateFlags& changedStates);

	private:

		D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineDesc;
		shared_ref<ID3D12PipelineState> d3dPipelineState;
		D3D12_PRIMITIVE_TOPOLOGY d3dPrimitiveTopology;
		std::vector<D3D12_INPUT_ELEMENT_DESC> d3dInputLayoutElements;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 resource binding
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxResourceBindingD3D12 : public GfxResourceBinding
	{
	public:

		GfxResourceBindingD3D12(GfxSystem &gfxSystem);

		virtual ~GfxResourceBindingD3D12();

		Result SetOnD3D12Context(GfxContextD3D12* gfxContextD3D12);

		inline ID3D12RootSignature* GetD3DRootSignature() const;

	protected:

		virtual Result OnInitialize();

	private:

		std::unique_ptr<GfxSystemD3D12::DescriptorSet> samplerDescriptorSet;
		std::vector<D3D12_DESCRIPTOR_RANGE> d3dDesriptorRangesCbvSrvUav;
		std::vector<D3D12_DESCRIPTOR_RANGE> d3dDesriptorRangesSampler;
		std::vector<D3D12_ROOT_PARAMETER> d3dRootParameters;
		shared_ref<ID3D12RootSignature> d3dRootSignature;
		shared_ref<ID3DBlob> d3dSerializedRootSignature;
		std::vector<std::unique_ptr<GfxSystemD3D12::DescriptorSet>> tableDescriptorSets;
		std::vector<ID3D12DescriptorHeap*> d3dTableDescriptorHeaps;
		ur_size tableIndexCbvSrvUav;
		ur_size tableIndexSampler;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Utilities
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	extern UR_DECL D3D12_RESOURCE_DESC GfxTextureDescToD3D12ResDesc(const GfxTextureDesc &desc);

	extern UR_DECL D3D12_RESOURCE_DESC GfxBufferDescToD3D12ResDesc(const GfxBufferDesc &desc);

	extern UR_DECL D3D12_RESOURCE_FLAGS GfxBindFlagsToD3D12ResFlags(const ur_uint gfxFlags);

	extern UR_DECL D3D12_HEAP_TYPE GfxUsageToD3D12HeapType(const GfxUsage gfxUsage);

	extern UR_DECL D3D12_RESOURCE_STATES GfxBindFlagsAndUsageToD3D12ResState(ur_uint gfxBindFlags, const GfxUsage gfxUsage);

	extern UR_DECL D3D12_SUBRESOURCE_DATA GfxResourceDataToD3D12(const GfxResourceData& gfxData);

	extern UR_DECL D3D12_RENDER_TARGET_BLEND_DESC GfxBlendStateToD3D12(const GfxBlendState& state);

	extern UR_DECL D3D12_BLEND GfxBlendFactorToD3D12(GfxBlendFactor blendFactor);

	extern UR_DECL D3D12_BLEND_OP GfxBlendOpToD3D12(GfxBlendOp blendOp);

	extern UR_DECL D3D12_RASTERIZER_DESC GfxRasterizerStateToD3D12(const GfxRasterizerState& state);

	extern UR_DECL D3D12_FILL_MODE GfxFillModeToD3D12(GfxFillMode mode);

	extern UR_DECL D3D12_CULL_MODE GfxCullModeToD3D12(GfxCullMode mode);

	extern UR_DECL D3D12_DEPTH_STENCIL_DESC GfxDepthStencilStateToD3D12(const GfxDepthStencilState &state);

	extern UR_DECL D3D12_COMPARISON_FUNC GfxCmpFuncToD3D12(GfxCmpFunc func);

	extern UR_DECL D3D12_STENCIL_OP GfxStencilOpToD3D12(GfxStencilOp stencilOp);

	extern UR_DECL D3D12_DEPTH_STENCILOP_DESC GfxDepthStencilOpDescToD3D12(const GfxDepthStencilOpDesc& desc);

	extern UR_DECL LPCSTR GfxSemanticToD3D12(GfxSemantic semantic);

	extern UR_DECL D3D12_INPUT_ELEMENT_DESC GfxInputElementToD3D12(const GfxInputElement &element);

	extern UR_DECL D3D12_PRIMITIVE_TOPOLOGY GfxPrimitiveTopologyToD3D12(GfxPrimitiveTopology topology);

	extern UR_DECL D3D12_PRIMITIVE_TOPOLOGY_TYPE GfxPrimitiveTopologyToD3D12Type(GfxPrimitiveTopology topology);

	extern UR_DECL D3D12_VIEWPORT GfxViewPortToD3D12(const GfxViewPort &viewPort);

	extern UR_DECL D3D12_RECT RectIToD3D12(const RectI &rect);

	extern UR_DECL D3D12_FILTER GfxFilterToD3D12(GfxFilter minFilter, GfxFilter magFilter, GfxFilter mipFilter);

	extern UR_DECL D3D12_TEXTURE_ADDRESS_MODE GfxTextureAddressModeToD3D12(GfxTextureAddressMode mode);

	extern UR_DECL D3D12_SAMPLER_DESC GfxSamplerStateToD3D12(const GfxSamplerState &state);

	extern UR_DECL HRESULT FillUploadBuffer(ID3D12Resource *uploadResource, ID3D12Resource *destinationResource, ur_uint firstSubresource, ur_uint numSubresources, const D3D12_SUBRESOURCE_DATA *srcData);

	extern UR_DECL HRESULT UpdateTextureSubresources(ID3D12GraphicsCommandList* commandList, ID3D12Resource* dstResource, ID3D12Resource* uploadResource,
		ur_uint dstSubresource, ur_uint srcSubresource, ur_uint numSubresources);

} // end namespace UnlimRealms

#include "Gfx/D3D12/GfxSystemD3D12.inline.h"