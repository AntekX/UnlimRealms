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

		virtual Result CreateVertexShader(std::unique_ptr<GfxVertexShader> &gfxVertexShader);

		virtual Result CreatePixelShader(std::unique_ptr<GfxPixelShader> &gfxPixelShader);

		virtual Result CreateInputLayout(std::unique_ptr<GfxInputLayout> &gfxInputLayout);

		virtual Result CreatePipelineState(std::unique_ptr<GfxPipelineState> &gfxPipelineState);

		Result InitializeFrameData(ur_uint framesCount);

		Result SetFrame(ur_uint frameIndex);

		Result SetNextFrame();

		Result WaitFrame(ur_uint frameIndex);

		Result WaitCurrentFrame();

		Result WaitGPU();

		Result AddCommandList(shared_ref<ID3D12CommandList> &d3dCommandList);

		inline ur_bool IsFrameComplete(ur_uint frameIndex);

		inline ur_bool IsCurrentFrameComplete();

		inline WinCanvas* GetWinCanvas() const;

		inline IDXGIFactory4* GetDXGIFactory() const;

		inline ID3D12Device* GetD3DDevice() const;
		
		inline ID3D12CommandQueue* GetD3DCommandQueue() const;

		inline ID3D12CommandAllocator* GetD3DCommandAllocator() const;


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Descriptor & Descriptors Heap
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		class UR_DECL DescriptorHeap;

		class UR_DECL Descriptor
		{
			friend class DescriptorHeap;
		public:

			Descriptor(DescriptorHeap* heap);

			~Descriptor();

			inline D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle() const;
			
			inline D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle() const;

		private:

			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
			DescriptorHeap* heap;
			ur_size heapIdx;
		};

		class UR_DECL DescriptorHeap : public GfxEntity
		{
		public:

			DescriptorHeap(GfxSystemD3D12& gfxSystem, D3D12_DESCRIPTOR_HEAP_TYPE d3dHeapType);

			~DescriptorHeap();

			Result AcquireDescriptor(std::unique_ptr<Descriptor>& descriptor);

			Result ReleaseDescriptor(Descriptor& descriptor);

		private:

			static const ur_size DescriptorsPerHeap = 256;

			typedef std::pair<ur_size, ur_size> Range;

			struct Page : public NonCopyable
			{
				shared_ref<ID3D12DescriptorHeap> d3dHeap;
				std::vector<Range> freeRanges; // todo: optimize ranges
			};

			D3D12_DESCRIPTOR_HEAP_TYPE d3dHeapType;
			std::mutex modifyMutex;
			std::vector<std::unique_ptr<Page>> pagePool;
			std::vector<Descriptor*> descriptors;
			ur_uint32 d3dDescriptorSize;
			ur_size currentPageIdx;
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
		ur_uint commandListsId;
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

		virtual Result SetPipelineState(GfxPipelineState *state);

		virtual Result SetTexture(GfxTexture *texture, ur_uint slot);

		virtual Result SetConstantBuffer(GfxBuffer *buffer, ur_uint slot);

		virtual Result SetVertexBuffer(GfxBuffer *buffer, ur_uint slot, ur_uint stride, ur_uint offset);

		virtual Result SetIndexBuffer(GfxBuffer *buffer, ur_uint bitsPerIndex, ur_uint offset);

		virtual Result SetVertexShader(GfxVertexShader *shader);

		virtual Result SetPixelShader(GfxPixelShader *shader);

		virtual Result Draw(ur_uint vertexCount, ur_uint vertexOffset, ur_uint instanceCount, ur_uint instanceOffset);

		virtual Result DrawIndexed(ur_uint indexCount, ur_uint indexOffset, ur_uint vertexOffset, ur_uint instanceCount, ur_uint instanceOffset);

		virtual Result UpdateBuffer(GfxBuffer *buffer, GfxGPUAccess gpuAccess, bool doNotWait, UpdateBufferCallback callback);

	private:

		shared_ref<ID3D12GraphicsCommandList> d3dCommandList;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics texture
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxTextureD3D12 : public GfxTexture
	{
	public:

		GfxTextureD3D12(GfxSystem &gfxSystem);

		virtual ~GfxTextureD3D12();

		Result Initialize(const GfxTextureDesc &desc, shared_ref<ID3D12Resource> &d3dTexture);

		inline ID3D12Resource* GetD3DResource() const;
		
		inline GfxSystemD3D12::Descriptor* GetSRVDescriptor() const;

	protected:

		virtual Result OnInitialize(const GfxResourceData *data);

	private:

		bool initializedFromD3DRes;
		shared_ref<ID3D12Resource> d3dResource;
		std::unique_ptr<GfxSystemD3D12::Descriptor> srvDescriptor;
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

	protected:

		class UR_DECL BackBuffer : public GfxRenderTargetD3D12
		{
		public:

			BackBuffer(GfxSystem &gfxSystem, shared_ref<ID3D12Resource> &dxgiSwapChainBuffer);

			virtual ~BackBuffer();

		protected:

			virtual Result InitializeTargetBuffer(std::unique_ptr<GfxTexture> &resultBuffer, const GfxTextureDesc &desc);

		private:

			shared_ref<ID3D12Resource> dxgiSwapChainBuffer;
		};

	private:

		DXGI_SWAP_CHAIN_DESC1 dxgiChainDesc;
		shared_ref<IDXGISwapChain3> dxgiSwapChain;
		ur_uint backBufferIndex;
		std::vector<std::unique_ptr<BackBuffer>> backBuffers;
		shared_ref<ID3D12GraphicsCommandList> d3dCommandList;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics buffer
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxBufferD3D12 : public GfxBuffer
	{
	public:

		GfxBufferD3D12(GfxSystem &gfxSystem);

		virtual ~GfxBufferD3D12();

	protected:

		virtual Result OnInitialize(const GfxResourceData *data);

	private:

		shared_ref<ID3D12Resource> d3dResource;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics vertex shader
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxVertexShaderD3D12 : public GfxVertexShader
	{
	public:

		GfxVertexShaderD3D12(GfxSystem &gfxSystem);

		virtual ~GfxVertexShaderD3D12();

	protected:

		virtual Result OnInitialize();

	private:

		// todo
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics pixel shader
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxPixelShaderD3D12 : public GfxPixelShader
	{
	public:

		GfxPixelShaderD3D12(GfxSystem &gfxSystem);

		virtual ~GfxPixelShaderD3D12();

	protected:

		virtual Result OnInitialize();

	private:

		// todo
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics input layout
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxInputLayoutD3D12 : public GfxInputLayout
	{
	public:

		GfxInputLayoutD3D12(GfxSystem &gfxSystem);

		virtual ~GfxInputLayoutD3D12();

	protected:

		virtual Result OnInitialize(const GfxShader &shader, const GfxInputElement *elements, ur_uint count);

	private:

		// todo
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct3D12 graphics pipeline state
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL GfxPipelineStateD3D12 : public GfxPipelineState
	{
	public:

		GfxPipelineStateD3D12(GfxSystem &gfxSystem);

		virtual ~GfxPipelineStateD3D12();

	protected:

		virtual Result OnSetRenderState(const GfxRenderState &renderState);

	private:

		// todo
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Utilities
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	extern UR_DECL D3D12_RESOURCE_DESC GfxTextureDescToD3D12ResDesc(const GfxTextureDesc &desc);

	extern UR_DECL D3D12_RESOURCE_DESC GfxBufferDescToD3D12ResDesc(const GfxBufferDesc &desc);

	extern UR_DECL D3D12_RESOURCE_FLAGS GfxBindFlagsToD3D12ResFlags(const ur_uint gfxFlags);

	extern UR_DECL D3D12_HEAP_TYPE GfxUsageToD3D12HeapType(const GfxUsage gfxUsage);

	extern UR_DECL D3D12_RESOURCE_STATES GfxBindFlagsAndUsageToD3D12ResState(ur_uint gfxBindFlags, const GfxUsage gfxUsage);

} // end namespace UnlimRealms

#include "Gfx/D3D12/GfxSystemD3D12.inline.h"