///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Sys/Log.h"
#include "Sys/Windows/WinCanvas.h"
#include "Gfx/D3D11/GfxSystemD3D11.h"
#include "Gfx/DXGIUtils/DXGIUtils.h"

namespace UnlimRealms
{
	
	static const D3D_FEATURE_LEVEL DefaultFeatureLevels[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxSystemD3D11
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxSystemD3D11::GfxSystemD3D11(Realm &realm) :
		GfxSystem(realm)
	{
		this->winCanvas = ur_null;
		this->d3dFeatureLevel = D3D_FEATURE_LEVEL(-1);
		this->commandListId = 0;
	}

	GfxSystemD3D11::~GfxSystemD3D11()
	{
		this->ReleaseDeviceObjects();
		this->ReleaseDXGIObjects();
	}

	Result GfxSystemD3D11::Initialize(Canvas *canvas)
	{
		if (ur_null == canvas)
			return ResultError(InvalidArgs, "GfxSystemD3D11: failed to initialize, target canvas is null");

		if (typeid(*canvas).hash_code() != typeid(WinCanvas).hash_code())
			return ResultError(InvalidArgs, "GfxSystemD3D11: failed to initialize, target canvas type is unsupported");
		
		this->winCanvas = static_cast<WinCanvas*>(canvas);

		Result res = this->InitializeDXGI();
		if (Succeeded(res))
		{
			res = this->InitializeDevice(ur_null, DefaultFeatureLevels, ARRAYSIZE(DefaultFeatureLevels));
		}

		return (Succeeded(res) ?
			ResultNote(Success, "GfxSystemD3D11: initialized") :
			ResultError(res.Code, "GfxSystemD3D11: failed to initialize"));
	}

	Result GfxSystemD3D11::Render()
	{
		if (this->d3dDevice.empty() || this->d3dImmediateContext.empty())
			return ResultError(NotInitialized, "GfxTextureD3D11: failed to initialize, device not ready");

		{ // swap command lists
			std::lock_guard<std::mutex> lock(this->commandListMutex);
			this->commandListId = !this->commandListId;
		}

		// execute command lists
		for (auto &d3dCommandList : this->d3dCommandLists[!this->commandListId])
		{
			this->d3dImmediateContext->ExecuteCommandList(d3dCommandList, FALSE);
		}
		this->d3dCommandLists[!this->commandListId].clear();

		return Result(Success);
	}

	Result GfxSystemD3D11::CreateContext(std::unique_ptr<GfxContext> &gfxContext)
	{
		gfxContext.reset(new GfxContextD3D11(*this));
		return Result(Success);
	}

	Result GfxSystemD3D11::CreateTexture(std::unique_ptr<GfxTexture> &gfxTexture)
	{
		gfxTexture.reset(new GfxTextureD3D11(*this));
		return Result(Success);
	}

	Result GfxSystemD3D11::CreateRenderTarget(std::unique_ptr<GfxRenderTarget> &gfxRT)
	{
		gfxRT.reset(new GfxRenderTargetD3D11(*this));
		return Result(Success);
	}

	Result GfxSystemD3D11::CreateSwapChain(std::unique_ptr<GfxSwapChain> &gfxSwapChain)
	{
		gfxSwapChain.reset(new GfxSwapChainD3D11(*this));
		return Result(Success);
	}

	Result GfxSystemD3D11::CreateBuffer(std::unique_ptr<GfxBuffer> &gfxBuffer)
	{
		gfxBuffer.reset(new GfxBufferD3D11(*this));
		return Result(Success);
	}

	Result GfxSystemD3D11::CreateVertexShader(std::unique_ptr<GfxVertexShader> &gfxVertexShader)
	{
		gfxVertexShader.reset(new GfxVertexShaderD3D11(*this));
		return Result(Success);
	}

	Result GfxSystemD3D11::CreatePixelShader(std::unique_ptr<GfxPixelShader> &gfxPixelShader)
	{
		gfxPixelShader.reset(new GfxPixelShaderD3D11(*this));
		return Result(Success);
	}

	Result GfxSystemD3D11::CreateSampler(std::unique_ptr<GfxSampler> &gfxSampler)
	{
		gfxSampler.reset(new GfxSamplerD3D11(*this));
		return Result(Success);
	}

	Result GfxSystemD3D11::CreateResourceBinding(std::unique_ptr<GfxResourceBinding> &gfxBinding)
	{
		gfxBinding.reset(new GfxResourceBindingD3D11(*this));
		return Result(Success);
	}

	Result GfxSystemD3D11::CreatePipelineStateObject(std::unique_ptr<GfxPipelineStateObject> &gfxPipelineState)
	{
		gfxPipelineState.reset(new GfxPipelineStateObjectD3D11(*this));
		return Result(Success);
	}

	Result GfxSystemD3D11::CreateInputLayout(std::unique_ptr<GfxInputLayout> &gfxInputLayout)
	{
		gfxInputLayout.reset(new GfxInputLayoutD3D11(*this));
		return Result(Success);
	}

	Result GfxSystemD3D11::CreatePipelineState(std::unique_ptr<GfxPipelineState> &gfxPipelineState)
	{
		gfxPipelineState.reset(new GfxPipelineStateD3D11(*this));
		return Result(Success);
	}

	Result GfxSystemD3D11::InitializeDXGI()
	{
		this->ReleaseDXGIObjects();

		HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&this->dxgiFactory));
		if (FAILED(hr))
			return ResultError(Failure, "GfxSystemD3D11: failed to create DXGI factory");

		UINT adapterId = 0;
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		do
		{
			shared_ref<IDXGIAdapter1> dxgiAdapter;
			hr = this->dxgiFactory->EnumAdapters1(adapterId++, dxgiAdapter);
			if (SUCCEEDED(hr))
			{
				dxgiAdapter->GetDesc1(&dxgiAdapterDesc);
				this->dxgiAdapters.push_back(dxgiAdapter);
				this->gfxAdapters.push_back(DXGIAdapterDescToGfx(dxgiAdapterDesc));
			}
		} while (SUCCEEDED(hr));

		return (!this->dxgiAdapters.empty() ?
			Result(Success) :
			ResultError(Failure, "GfxSystemD3D11: failed to enumerate adapaters"));
	}

	Result GfxSystemD3D11::InitializeDevice(IDXGIAdapter *pAdapter,
		const D3D_FEATURE_LEVEL *featureLevels,
		UINT levelsCount)
	{
		this->ReleaseDeviceObjects();

		// create D3D device and immediate context

		if (this->dxgiAdapters.empty())
			return ResultError(Failure, "GfxSystemD3D11: failed to initialize device, adapter(s) not found");

		D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_UNKNOWN;
		UINT flags = 0;
		#ifdef _DEBUG
		//flags |= D3D11_CREATE_DEVICE_DEBUG; // in Windows 10 the Windows SDK does not install the Direct3D debug layer
		#endif

		if (ur_null == pAdapter)
		{
			this->gfxAdapterIdx = 0;
			// try to find high performance GPU
			DXGI_ADAPTER_DESC1 adapterDesc;
			for (ur_size idx = 0; idx < this->dxgiAdapters.size(); ++idx)
			{
				this->dxgiAdapters[idx]->GetDesc1(&adapterDesc);
				ur_size dedicatedMemoryMb = adapterDesc.DedicatedVideoMemory / (1 << 20);
				if (dedicatedMemoryMb > 1024) // let's consider that HP GPU has at least that much VRAM on board
				{
					this->gfxAdapterIdx = ur_uint(idx);
					break;
				}
			}
			pAdapter = *(this->dxgiAdapters.begin() + this->gfxAdapterIdx);
		}

		HRESULT hr = D3D11CreateDevice(pAdapter, driverType, ur_null, flags, featureLevels, levelsCount, D3D11_SDK_VERSION,
			this->d3dDevice, &this->d3dFeatureLevel, this->d3dImmediateContext);

		return (SUCCEEDED(hr) ?
			Result(Success) :
			ResultError(Failure, "GfxSystemD3D11: failed to initialize Direct3D device"));
	}

	void GfxSystemD3D11::ReleaseDXGIObjects()
	{
		this->gfxAdapters.clear();
		this->dxgiAdapters.clear();
		this->dxgiFactory.reset(ur_null);
	}

	void GfxSystemD3D11::ReleaseDeviceObjects()
	{
		this->d3dImmediateContext.reset(ur_null);
		this->d3dDevice.reset(ur_null);
		this->d3dFeatureLevel = D3D_FEATURE_LEVEL(-1);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxRenderTargetD3D11
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	GfxContextD3D11::GfxContextD3D11(GfxSystem &gfxSystem) :
		GfxContext(gfxSystem)
	{
	}

	GfxContextD3D11::~GfxContextD3D11()
	{
	}

	Result GfxContextD3D11::Initialize()
	{
		this->d3dContext = ur_null;

		GfxSystemD3D11 &d3dSystem = static_cast<GfxSystemD3D11&>(this->GetGfxSystem());
		ID3D11Device *d3dDevice = d3dSystem.GetDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxContextD3D11: failed to initialize, device not ready");

		HRESULT hr = d3dDevice->CreateDeferredContext(0, this->d3dContext);
		if (FAILED(hr))
			return ResultError(Failure, "GfxContextD3D11: failed to create device context");

		return Result(Success);
	}

	Result GfxContextD3D11::Begin()
	{
		// nothing to do
		return Result(Success);
	}

	Result GfxContextD3D11::End()
	{
		if (this->d3dContext.empty())
			return ResultError(NotInitialized, "GfxContextD3D11::End: failed, device context is not ready");

		shared_ref<ID3D11CommandList> d3dCommandList;
		HRESULT hr = this->d3dContext->FinishCommandList(FALSE, d3dCommandList);
		if (FAILED(hr))
			return ResultError(Failure, "GfxContextD3D11::End: failed to finish command list");

		GfxSystemD3D11 &d3dSystem = static_cast<GfxSystemD3D11&>(this->GetGfxSystem());
		d3dSystem.AddCommandList(d3dCommandList);

		return Result(Success);
	}

	Result GfxContextD3D11::SetRenderTarget(GfxRenderTarget *rt)
	{
		return this->SetRenderTarget(rt, rt);
	}

	Result GfxContextD3D11::SetRenderTarget(GfxRenderTarget *rt, GfxRenderTarget *ds)
	{
		if (this->d3dContext.empty())
			return ResultError(Failure, "GfxContextD3D11::SetRenderTarget: failed, device context is not ready");

		ID3D11RenderTargetView *rtView[] = { rt != ur_null ?
			static_cast<GfxRenderTargetD3D11*>(rt)->GetRTView() :
			ur_null };
		ID3D11DepthStencilView *dsView = { ds != ur_null ?
			static_cast<GfxRenderTargetD3D11*>(ds)->GetDSView() :
			ur_null };
		this->d3dContext->OMSetRenderTargets(1, rtView, dsView);

		if (rt != ur_null)
		{
			GfxTexture *rtTex = rt->GetTargetBuffer();
			if (ur_null == rtTex && ds != ur_null) rtTex = ds->GetDepthStencilBuffer();
			if (rtTex != ur_null)
			{
				GfxViewPort gfxViewPort = {
					0.0f, 0.0f, (ur_float)rtTex->GetDesc().Width, (ur_float)rtTex->GetDesc().Height, 0.0f, 1.0f
				};
				this->SetViewPort(&gfxViewPort);
			}
		}

		return Result(Success);
	}

	Result GfxContextD3D11::SetViewPort(const GfxViewPort *viewPort)
	{
		if (this->d3dContext.empty())
			return ResultError(Failure, "GfxContextD3D11::SetViewPort: failed, device context is not ready");

		this->d3dContext->RSSetViewports(1, viewPort != ur_null ?
			&GfxViewPortToD3D11(*viewPort) : ur_null);

		return Result(Success);
	}

	Result GfxContextD3D11::GetViewPort(GfxViewPort &viewPort)
	{
		if (this->d3dContext.empty())
			return ResultError(Failure, "GfxContextD3D11::GetViewPort: failed, device context is not ready");
		
		UINT numViewports = 1;
		D3D11_VIEWPORT d3dViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
		this->d3dContext->RSGetViewports(&numViewports, d3dViewports);
		if (0 == numViewports)
			return Result(NotInitialized);
		
		viewPort = GfxViewPortFromD3D11(d3dViewports[0]);

		return Result(Success);
	}

	Result GfxContextD3D11::SetScissorRect(const RectI *rect)
	{
		if (this->d3dContext.empty())
			return ResultError(Failure, "GfxContextD3D11::SetScissorRects: failed, device context is not ready");

		ur_uint numRects = (rect != ur_null ? 1 : 0);
		D3D11_RECT d3dRect;
		if (numRects > 0) d3dRect = RectIToD3D11(*rect);
		this->d3dContext->RSSetScissorRects(numRects, (numRects > 0 ? &d3dRect : ur_null));

		return Result(Success);
	}

	Result GfxContextD3D11::ClearTarget(GfxRenderTarget *rt,
		bool clearColor, const ur_float4 &color,
		bool clearDepth, const ur_float &depth,
		bool clearStencil, const ur_uint &stencil)
	{
		GfxRenderTargetD3D11 *rt_d3d11 = static_cast<GfxRenderTargetD3D11*>(rt);
		if (ur_null == rt_d3d11)
			return ResultError(Failure, "GfxContextD3D11::ClearTarget: failed, invalid render target");

		if (this->d3dContext.empty())
			return ResultError(Failure, "GfxContextD3D11::ClearTarget: failed, device context is not ready");

		if (clearColor)
		{
			this->d3dContext->ClearRenderTargetView(rt_d3d11->GetRTView(), &color.x);
		}

		if (clearDepth || clearStencil)
		{
			UINT flags = 0 |
				(clearDepth ? D3D11_CLEAR_DEPTH : 0) |
				(clearStencil ? D3D11_CLEAR_STENCIL : 0);
			this->d3dContext->ClearDepthStencilView(rt_d3d11->GetDSView(), flags, depth, stencil);
		}

		return Result(Success);
	}

	Result GfxContextD3D11::SetPipelineStateObject(GfxPipelineStateObject *state)
	{
		// TODO:
		return Result(NotImplemented);
	}

	Result GfxContextD3D11::SetResourceBinding(GfxResourceBinding *binding)
	{
		// TODO:
		return Result(NotImplemented);
	}
	
	Result GfxContextD3D11::SetPipelineState(GfxPipelineState *state)
	{
		if (this->d3dContext.empty())
			return ResultError(Failure, "GfxContextD3D11::SetPipelineState: failed, device context is not ready");

		GfxPipelineStateD3D11 *state_d3d11 = static_cast<GfxPipelineStateD3D11*>(state);
		if (ur_null == state_d3d11)
			return ResultError(Failure, "GfxContextD3D11::ClearTarget: failed, invalid render target");

		if (ur_null == state)
			return Result(InvalidArgs);
		
		d3dContext->IASetPrimitiveTopology(GfxPrimitiveTopologyToD3D11(state_d3d11->PrimitiveTopology));

		if (state->InputLayout != ur_null)
		{
			GfxInputLayoutD3D11 *layout_d3d11 = static_cast<GfxInputLayoutD3D11*>(state->InputLayout);
			d3dContext->IASetInputLayout(layout_d3d11->GetD3DInputLayout());
		}

		ID3D11SamplerState *d3dSamplers[GfxRenderState::MaxSamplerStates];
		for (ur_uint i = 0; i < GfxRenderState::MaxSamplerStates; ++i)
		{
			d3dSamplers[i] = state_d3d11->GetD3DSamplerState(i);
		};

		if (state->VertexShader != ur_null)
		{
			GfxVertexShaderD3D11 *vs_d3d11 = static_cast<GfxVertexShaderD3D11*>(state->VertexShader);
			d3dContext->VSSetShader(vs_d3d11->GetD3DShader(), ur_null, 0);
			d3dContext->VSSetSamplers(0, GfxRenderState::MaxSamplerStates, d3dSamplers);
		}

		if (state->PixelShader != ur_null)
		{
			GfxPixelShaderD3D11 *ps_d3d11 = static_cast<GfxPixelShaderD3D11*>(state->PixelShader);
			d3dContext->PSSetShader(ps_d3d11->GetD3DShader(), ur_null, 0);
			d3dContext->PSSetSamplers(0, GfxRenderState::MaxSamplerStates, d3dSamplers);
		}

		d3dContext->RSSetState(state_d3d11->GetD3DRasterizerState());

		d3dContext->OMSetDepthStencilState(state_d3d11->GetD3DDepthStencilState(), (UINT)state_d3d11->StencilRef);

		d3dContext->OMSetBlendState(state_d3d11->GetD3DBlendState(), ur_null, 0xffffffff);

		return Result(Success);
	}

	Result GfxContextD3D11::SetTexture(GfxTexture *texture, ur_uint slot)
	{
		if (this->d3dContext.empty())
			return ResultError(Failure, "GfxContextD3D11::SetConstantBuffer: failed, device context is not ready");

		GfxTextureD3D11 *gfxTextureD3D11 = static_cast<GfxTextureD3D11*>(texture);
		ur_uint numResources = (gfxTextureD3D11 != ur_null ? 1 : 0);
		ID3D11ShaderResourceView *srvs[] = { gfxTextureD3D11 != ur_null ? gfxTextureD3D11->GetSRV() : ur_null };
		this->d3dContext->VSSetShaderResources(slot, numResources, srvs);
		this->d3dContext->GSSetShaderResources(slot, numResources, srvs);
		this->d3dContext->HSSetShaderResources(slot, numResources, srvs);
		this->d3dContext->DSSetShaderResources(slot, numResources, srvs);
		this->d3dContext->PSSetShaderResources(slot, numResources, srvs);
		this->d3dContext->CSSetShaderResources(slot, numResources, srvs);

		return Result(Success);
	}

	Result GfxContextD3D11::SetConstantBuffer(GfxBuffer *buffer, ur_uint slot)
	{
		if (this->d3dContext.empty())
			return ResultError(Failure, "GfxContextD3D11::SetConstantBuffer: failed, device context is not ready");

		GfxBufferD3D11 *gfxBufferD3D11 = static_cast<GfxBufferD3D11*>(buffer);
		ur_uint numBuffers = (gfxBufferD3D11 != ur_null ? 1 : 0);
		ID3D11Buffer* buffers[] = { gfxBufferD3D11 != ur_null ? gfxBufferD3D11->GetD3DBuffer() : ur_null };
		this->d3dContext->VSSetConstantBuffers(slot, numBuffers, buffers);
		this->d3dContext->GSSetConstantBuffers(slot, numBuffers, buffers);
		this->d3dContext->HSSetConstantBuffers(slot, numBuffers, buffers);
		this->d3dContext->DSSetConstantBuffers(slot, numBuffers, buffers);
		this->d3dContext->PSSetConstantBuffers(slot, numBuffers, buffers);
		this->d3dContext->CSSetConstantBuffers(slot, numBuffers, buffers);

		return Result(Success);
	}

	Result GfxContextD3D11::SetVertexBuffer(GfxBuffer *buffer, ur_uint slot)
	{
		if (this->d3dContext.empty())
			return ResultError(Failure, "GfxContextD3D11::SetVertexBuffer: failed, device context is not ready");

		GfxBufferD3D11 *gfxBufferD3D11 = static_cast<GfxBufferD3D11*>(buffer);
		ur_uint numBuffers = (gfxBufferD3D11 != ur_null ? 1 : 0);
		ID3D11Buffer* buffers[] = { gfxBufferD3D11 != ur_null ? gfxBufferD3D11->GetD3DBuffer() : ur_null };
		ur_uint strides[] = { gfxBufferD3D11 != ur_null ? gfxBufferD3D11->GetDesc().ElementSize : 0};
		ur_uint offsets[] = { 0 };
		this->d3dContext->IASetVertexBuffers(slot, numBuffers, buffers, strides, offsets);

		return Result(Success);
	}

	Result GfxContextD3D11::SetIndexBuffer(GfxBuffer *buffer)
	{
		if (this->d3dContext.empty())
			return ResultError(Failure, "GfxContextD3D11::SetIndexBuffer: failed, device context is not ready");

		ur_uint bitsPerIndex = (buffer != ur_null ? buffer->GetDesc().ElementSize * 8 : 0);
		DXGI_FORMAT indexFmt = GfxBitsPerIndexToDXGIFormat(bitsPerIndex);
		if (DXGI_FORMAT_UNKNOWN == indexFmt)
			return ResultError(Failure, "GfxContextD3D11::SetIndexBuffer: invalid bits per index valaue");

		GfxBufferD3D11 *gfxBufferD3D11 = static_cast<GfxBufferD3D11*>(buffer);
		this->d3dContext->IASetIndexBuffer(
			gfxBufferD3D11 != ur_null ? gfxBufferD3D11->GetD3DBuffer() : ur_null,
			indexFmt, 0);

		return Result(Success);
	}

	Result GfxContextD3D11::SetVertexShader(GfxVertexShader *shader)
	{
		if (this->d3dContext.empty())
			return ResultError(Failure, "GfxContextD3D11::SetVertexShader: failed, device context is not ready");

		GfxVertexShaderD3D11 *gfxShaderD3D11 = static_cast<GfxVertexShaderD3D11*>(shader);
		this->d3dContext->VSSetShader(
			gfxShaderD3D11 != ur_null ? gfxShaderD3D11->GetD3DShader() : ur_null,
			ur_null, 0);

		return Result(Success);
	}

	Result GfxContextD3D11::SetPixelShader(GfxPixelShader *shader)
	{
		if (this->d3dContext.empty())
			return ResultError(Failure, "GfxContextD3D11::SetPixelShader: failed, device context is not ready");

		GfxPixelShaderD3D11 *gfxShaderD3D11 = static_cast<GfxPixelShaderD3D11*>(shader);
		this->d3dContext->PSSetShader(
			gfxShaderD3D11 != ur_null ? gfxShaderD3D11->GetD3DShader() : ur_null,
			ur_null, 0);

		return Result(Success);
	}

	Result GfxContextD3D11::Draw(ur_uint vertexCount, ur_uint vertexOffset, ur_uint instanceCount, ur_uint instanceOffset)
	{
		if (this->d3dContext.empty())
			return Result(Failure);

		if (instanceCount > 0)
		{
			this->d3dContext->DrawInstanced((UINT)vertexCount, (UINT)instanceCount, (UINT)vertexOffset, (UINT)instanceOffset);
		}
		else
		{
			this->d3dContext->Draw((UINT)vertexCount, (UINT)vertexOffset);
		}

		return Result(Success);
	}

	Result GfxContextD3D11::DrawIndexed(ur_uint indexCount, ur_uint indexOffset, ur_uint vertexOffset, ur_uint instanceCount, ur_uint instanceOffset)
	{
		if (this->d3dContext.empty())
			return Result(Failure);

		if (instanceCount > 0)
		{
			this->d3dContext->DrawIndexedInstanced((UINT)indexCount, (UINT)instanceCount, (UINT)indexOffset, (UINT)vertexOffset, (UINT)instanceOffset);
		}
		else
		{
			this->d3dContext->DrawIndexed((UINT)indexCount, (UINT)indexOffset, (UINT)vertexOffset);
		}

		return Result(Success);
	}

	Result GfxContextD3D11::UpdateBuffer(GfxBuffer *buffer, GfxGPUAccess gpuAccess, UpdateBufferCallback callback)
	{
		if (this->d3dContext.empty())
			return ResultError(Failure, "GfxContextD3D11::UpdateBuffer: failed, device context is not ready");

		GfxBufferD3D11 *gfxBufferD3D11 = static_cast<GfxBufferD3D11*>(buffer);
		if (ur_null == gfxBufferD3D11 || ur_null == gfxBufferD3D11->GetD3DBuffer())
			return ResultError(InvalidArgs, "GfxContextD3D11::UpdateBuffer: failed, invalid buffer");

		D3D11_MAP d3dMapType = GfxGPUAccessFlagToD3D11(gpuAccess);
		BOOL doNotWait = FALSE;
		UINT d3dMapFlags = (doNotWait ? D3D11_MAP_FLAG_DO_NOT_WAIT : 0);
		D3D11_MAPPED_SUBRESOURCE d3dMappedRes;
		HRESULT hr = this->d3dContext->Map(gfxBufferD3D11->GetD3DBuffer(), 0, d3dMapType, d3dMapFlags, &d3dMappedRes);
		if (FAILED(hr))
			return ResultError(Failure, "GfxContextD3D11::UpdateBuffer: failed to map resource");

		GfxResourceData gfxData;
		gfxData.Ptr = d3dMappedRes.pData;
		gfxData.RowPitch = (ur_uint)d3dMappedRes.RowPitch;
		gfxData.SlicePitch = (ur_uint)d3dMappedRes.DepthPitch;
		
		Result res = callback(&gfxData);

		this->d3dContext->Unmap(gfxBufferD3D11->GetD3DBuffer(), 0);

		return res;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxTextureD3D11
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	
	GfxTextureD3D11::GfxTextureD3D11(GfxSystem &gfxSystem) :
		GfxTexture(gfxSystem),
		initializedFromD3DTex(false)
	{

	}

	GfxTextureD3D11::~GfxTextureD3D11()
	{
	}

	Result GfxTextureD3D11::Initialize(const GfxTextureDesc &desc, shared_ref<ID3D11Texture2D> &d3dTexture)
	{
		this->initializedFromD3DTex = true; // setting this flag to skip OnInitialize
		GfxTexture::Initialize(desc, ur_null); // store desc in the base class, 
		this->initializedFromD3DTex = false;

		this->d3dTexture.reset(ur_null);
		this->d3dSRV.reset(ur_null);

		GfxSystemD3D11 &d3dSystem = static_cast<GfxSystemD3D11&>(this->GetGfxSystem());
		ID3D11Device *d3dDevice = d3dSystem.GetDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxTextureD3D11: failed to initialize, device not ready");

		this->d3dTexture.reset(d3dTexture);

		if (desc.BindFlags & ur_uint(GfxBindFlag::ShaderResource))
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC d3dSrvDesc;
			d3dSrvDesc.Format = GfxFormatToDXGI(this->GetDesc().Format, this->GetDesc().FormatView);
			d3dSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			d3dSrvDesc.Texture2D.MipLevels = -1;
			d3dSrvDesc.Texture2D.MostDetailedMip = 0;
			HRESULT hr = d3dDevice->CreateShaderResourceView(this->d3dTexture, &d3dSrvDesc, this->d3dSRV);
			if (FAILED(hr))
			{
				this->d3dTexture.reset(ur_null);
				return ResultError(Failure, "GfxTextureD3D11: failed to create SRV");
			}
		}

		return Result(Success);
	}

	Result GfxTextureD3D11::OnInitialize(const GfxResourceData *data)
	{
		// if texture is being initialized from d3d object - all the stuff is done in GfxTextureD3D11::Initialize
		// so skip this function as its results will be overriden anyway
		if (this->initializedFromD3DTex)
			return Result(Success);

		this->d3dTexture.reset(ur_null);
		this->d3dSRV.reset(ur_null);

		GfxSystemD3D11 &d3dSystem = static_cast<GfxSystemD3D11&>(this->GetGfxSystem());
		ID3D11Device *d3dDevice = d3dSystem.GetDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxTextureD3D11: failed to initialize, device not ready");

		std::vector<D3D11_SUBRESOURCE_DATA> d3dData;
		D3D11_SUBRESOURCE_DATA *d3dDataPtr = ur_null; 
		if (data != ur_null)
		{
			ur_uint levels = this->GetDesc().Levels;
			d3dData.resize(std::max(ur_uint(1), levels));
			for (ur_uint i = 0; i < levels; ++i)
			{
				d3dData[i].pSysMem = data[i].Ptr;
				d3dData[i].SysMemPitch = data[i].RowPitch;
				d3dData[i].SysMemSlicePitch = data[i].SlicePitch;
			}
			d3dDataPtr = d3dData.data();
		}

		D3D11_TEXTURE2D_DESC d3dDesc = GfxTextureDescToD3D11(this->GetDesc());
		d3dDesc.Format = GfxFormatToDXGI(this->GetDesc().Format, GfxFormatView::Typeless);
		HRESULT hr = d3dDevice->CreateTexture2D(&d3dDesc, d3dDataPtr, this->d3dTexture);
		if (FAILED(hr))
			return ResultError(Failure, "GfxTextureD3D11: failed to create texture");
		
		if (this->GetDesc().BindFlags & ur_uint(GfxBindFlag::ShaderResource))
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC d3dSrvDesc;
			d3dSrvDesc.Format = GfxFormatToDXGI(this->GetDesc().Format, this->GetDesc().FormatView);
			d3dSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			d3dSrvDesc.Texture2D.MipLevels = -1;
			d3dSrvDesc.Texture2D.MostDetailedMip = 0;
			hr = d3dDevice->CreateShaderResourceView(this->d3dTexture, &d3dSrvDesc, this->d3dSRV);
			if (FAILED(hr))
			{
				this->d3dTexture.reset(ur_null);
				return ResultError(Failure, "GfxTextureD3D11: failed to create SRV");
			}
		}

		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxRenderTargetD3D11
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxRenderTargetD3D11::GfxRenderTargetD3D11(GfxSystem &gfxSystem) :
		GfxRenderTarget(gfxSystem)
	{
	}

	GfxRenderTargetD3D11::~GfxRenderTargetD3D11()
	{
	}

	Result GfxRenderTargetD3D11::OnInitialize()
	{
		this->d3dRTView.reset(ur_null);
		this->d3dDSView.reset(ur_null);

		GfxSystemD3D11 &d3dSystem = static_cast<GfxSystemD3D11&>(this->GetGfxSystem());
		ID3D11Device *d3dDevice = d3dSystem.GetDevice();

		GfxTextureD3D11 *d3dTargetBuffer = static_cast<GfxTextureD3D11*>(this->GetTargetBuffer());
		if (d3dTargetBuffer != ur_null)
		{
			const GfxTextureDesc &bufferDesc = this->GetTargetBuffer()->GetDesc();
			D3D11_RENDER_TARGET_VIEW_DESC d3dRTViewDesc;
			d3dRTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			d3dRTViewDesc.Format = GfxFormatToDXGI(bufferDesc.Format, bufferDesc.FormatView);
			d3dRTViewDesc.Texture2D.MipSlice = 0;

			HRESULT hr = d3dDevice->CreateRenderTargetView(d3dTargetBuffer->GetD3DTexture(), &d3dRTViewDesc, this->d3dRTView);
			if (FAILED(hr))
				return ResultError(Failure, "GfxRenderTargetD3D11: failed to create render target view");
		}

		GfxTextureD3D11 *d3dDepthStencilBuffer = static_cast<GfxTextureD3D11*>(this->GetDepthStencilBuffer());
		if (d3dDepthStencilBuffer != ur_null)
		{
			const GfxTextureDesc &bufferDesc = this->GetDepthStencilBuffer()->GetDesc();
			D3D11_DEPTH_STENCIL_VIEW_DESC d3dDSViewDesc;
			d3dDSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			switch (bufferDesc.Format)
			{
			case GfxFormat::R32G8X24: 
				d3dDSViewDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
				break;
			case GfxFormat::R32:
				d3dDSViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
				break;
			case GfxFormat::R24G8:
				d3dDSViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				break;
			case GfxFormat::R16:
				d3dDSViewDesc.Format = DXGI_FORMAT_D16_UNORM;
				break;
			default:
				d3dDSViewDesc.Format = DXGI_FORMAT_UNKNOWN;
			}
			d3dDSViewDesc.Flags = 0;
			d3dDSViewDesc.Texture2D.MipSlice = 0;

			HRESULT hr = d3dDevice->CreateDepthStencilView(d3dDepthStencilBuffer->GetD3DTexture(), &d3dDSViewDesc, this->d3dDSView);
			if (FAILED(hr))
				return ResultError(Failure, "GfxRenderTargetD3D11: failed to create depth stencil view");
		}

		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxSwapChainD3D11
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxSwapChainD3D11::GfxSwapChainD3D11(GfxSystem &gfxSystem) :
		GfxSwapChain(gfxSystem)
	{
		memset(&this->dxgiChainDesc, 0, sizeof(this->dxgiChainDesc));
	}

	GfxSwapChainD3D11::~GfxSwapChainD3D11()
	{
	}

	Result GfxSwapChainD3D11::Initialize(const GfxPresentParams &params)
	{
		this->targetBuffer.reset(ur_null);

		GfxSystemD3D11 &d3dSystem = static_cast<GfxSystemD3D11&>(this->GetGfxSystem());
		IDXGIFactory1 *dxgiFactory = d3dSystem.GetDXGIFactory();
		ID3D11Device *d3dDevice = d3dSystem.GetDevice();
		if (ur_null == d3dSystem.GetWinCanvas())
			return ResultError(NotInitialized, "GfxSwapChainD3D11::Initialize: failed, canvas not initialized");

		this->dxgiChainDesc.OutputWindow = d3dSystem.GetWinCanvas()->GetHwnd();
		this->dxgiChainDesc.Windowed = !params.Fullscreen;
		this->dxgiChainDesc.BufferDesc.Width = params.BufferWidth;
		this->dxgiChainDesc.BufferDesc.Height = params.BufferHeight;
		this->dxgiChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		this->dxgiChainDesc.BufferCount = params.BufferCount;
		this->dxgiChainDesc.BufferDesc.Format = GfxFormatToDXGI(params.BufferFormat, GfxFormatView::Unorm);
		this->dxgiChainDesc.BufferDesc.RefreshRate.Numerator = params.FullscreenRefreshRate;
		this->dxgiChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		this->dxgiChainDesc.SampleDesc.Count = params.MutisampleCount;
		this->dxgiChainDesc.SampleDesc.Quality = params.MutisampleQuality;
		this->dxgiChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		HRESULT hr = dxgiFactory->CreateSwapChain(d3dDevice, &this->dxgiChainDesc, this->dxgiSwapChain);
		if (FAILED(hr))
			return ResultError(Failure, "GfxSwapChainD3D11::Initialize: failed to create DXGI swap chain");

		shared_ref<ID3D11Texture2D> d3dTargetBuffer;
		hr = this->dxgiSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)d3dTargetBuffer);
		if (FAILED(hr))
			return ResultError(Failure, "GfxSwapChainD3D11::Initialize: failed to retrieve buffer from swap chain");

		GfxTextureDesc desc;
		desc.Width = params.BufferWidth;
		desc.Height = params.BufferHeight;
		desc.Levels = 1;
		desc.Format = params.BufferFormat;
		desc.FormatView = GfxFormatView::Unorm;
		desc.Usage = GfxUsage::Default;
		desc.BindFlags = ur_uint(GfxBindFlag::RenderTarget);
		desc.AccessFlags = ur_uint(0);

		std::unique_ptr<GfxSwapChainD3D11::RenderTarget> newRenderTarget(new RenderTarget(d3dSystem, d3dTargetBuffer));
		Result res = newRenderTarget->Initialize(desc, params.DepthStencilEnabled, params.DepthStencilFormat);
		if (Failed(res))
			return ResultError(res.Code, "GfxSwapChainD3D11::Initialize: failed to initialize render target");

		this->targetBuffer = std::move(newRenderTarget);

		return Result(Success);
	}

	Result GfxSwapChainD3D11::Present()
	{
		if (this->dxgiSwapChain.empty())
			return ResultError(NotInitialized, "GfxSwapChainD3D11::Present: DXGI swap chain not initialized");

		if (FAILED(this->dxgiSwapChain->Present(0, 0)))
			return ResultError(Failure, "GfxSwapChainD3D11::Present: failed");

		return Result(Success);
	}

	GfxRenderTarget* GfxSwapChainD3D11::GetTargetBuffer()
	{
		return this->targetBuffer.get();
	}

	GfxSwapChainD3D11::RenderTarget::RenderTarget(GfxSystem &gfxSystem, shared_ref<ID3D11Texture2D> &dxgiSwapChainBuffer) :
		GfxRenderTargetD3D11(gfxSystem),
		dxgiSwapChainBuffer(dxgiSwapChainBuffer)
	{
	}

	GfxSwapChainD3D11::RenderTarget::~RenderTarget()
	{
	}

	Result GfxSwapChainD3D11::RenderTarget::InitializeTargetBuffer(std::unique_ptr<GfxTexture> &resultBuffer, const GfxTextureDesc &desc)
	{
		GfxSystemD3D11 &d3dSystem = static_cast<GfxSystemD3D11&>(this->GetGfxSystem());
		
		std::unique_ptr<GfxTextureD3D11> newTargetBuffer(new GfxTextureD3D11(d3dSystem));
		Result res = newTargetBuffer->Initialize(desc, this->dxgiSwapChainBuffer);
		if (Failed(res))
			return ResultError(res.Code, "GfxSwapChainD3D11::RenderTarget::InitializeTargetBuffer: failed");
		
		resultBuffer = std::move(newTargetBuffer);

		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxBufferD3D11
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GfxBufferD3D11::GfxBufferD3D11(GfxSystem &gfxSystem) :
		GfxBuffer(gfxSystem)
	{
	}

	GfxBufferD3D11::~GfxBufferD3D11()
	{
	}

	Result GfxBufferD3D11::OnInitialize(const GfxResourceData *data)
	{
		this->d3dBuffer.reset(ur_null);

		GfxSystemD3D11 &d3dSystem = static_cast<GfxSystemD3D11&>(this->GetGfxSystem());
		ID3D11Device *d3dDevice = d3dSystem.GetDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxBufferD3D11: failed to initialize, device not ready");

		D3D11_SUBRESOURCE_DATA d3dData;
		D3D11_SUBRESOURCE_DATA *d3dDataPtr = ur_null;
		if (data != ur_null)
		{
			d3dData.pSysMem = data->Ptr;
			d3dData.SysMemPitch = data->RowPitch;
			d3dData.SysMemSlicePitch = data->SlicePitch;
			d3dDataPtr = &d3dData;
		}

		D3D11_BUFFER_DESC d3dDesc = GfxBufferDescToD3D11(this->GetDesc());
		HRESULT hr = d3dDevice->CreateBuffer(&d3dDesc, d3dDataPtr, this->d3dBuffer);
		if (FAILED(hr))
			return ResultError(Failure, "GfxBufferD3D11: failed to create buffer");

		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxVertexShaderD3D11
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GfxVertexShaderD3D11::GfxVertexShaderD3D11(GfxSystem &gfxSystem) :
		GfxVertexShader(gfxSystem)
	{
	}

	GfxVertexShaderD3D11::~GfxVertexShaderD3D11()
	{
	}

	Result GfxVertexShaderD3D11::OnInitialize()
	{
		this->d3dVertexShader.reset(ur_null);

		GfxSystemD3D11 &d3dSystem = static_cast<GfxSystemD3D11&>(this->GetGfxSystem());
		ID3D11Device *d3dDevice = d3dSystem.GetDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxVertexShaderD3D11: failed to initialize, device not ready");

		HRESULT hr = d3dDevice->CreateVertexShader(this->GetByteCode(), this->GetSizeInBytes(), ur_null, this->d3dVertexShader);
		if (FAILED(hr))
			return ResultError(Failure, "GfxVertexShaderD3D11: failed to create shader");

		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxPixelShaderD3D11
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GfxPixelShaderD3D11::GfxPixelShaderD3D11(GfxSystem &gfxSystem) :
		GfxPixelShader(gfxSystem)
	{
	}

	GfxPixelShaderD3D11::~GfxPixelShaderD3D11()
	{
	}

	Result GfxPixelShaderD3D11::OnInitialize()
	{
		this->d3dPixelShader.reset(ur_null);

		GfxSystemD3D11 &d3dSystem = static_cast<GfxSystemD3D11&>(this->GetGfxSystem());
		ID3D11Device *d3dDevice = d3dSystem.GetDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxPixelShaderD3D11: failed to initialize, device not ready");

		HRESULT hr = d3dDevice->CreatePixelShader(this->GetByteCode(), this->GetSizeInBytes(), ur_null, this->d3dPixelShader);
		if (FAILED(hr))
			return ResultError(Failure, "GfxPixelShaderD3D11: failed to create shader");

		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxInputLayoutD3D11
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GfxInputLayoutD3D11::GfxInputLayoutD3D11(GfxSystem &gfxSystem) :
		GfxInputLayout(gfxSystem)
	{
	}

	GfxInputLayoutD3D11::~GfxInputLayoutD3D11()
	{
	}

	Result GfxInputLayoutD3D11::OnInitialize(const GfxShader &shader, const GfxInputElement *elements, ur_uint count)
	{
		this->d3dInputLayout.reset(ur_null);

		GfxSystemD3D11 &d3dSystem = static_cast<GfxSystemD3D11&>(this->GetGfxSystem());
		ID3D11Device *d3dDevice = d3dSystem.GetDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxInputLayoutD3D11: failed to initialize, device not ready");

		std::vector<D3D11_INPUT_ELEMENT_DESC> d3dElements(count);
		const GfxInputElement *gfxElement = elements;
		for (auto &d3dElement : d3dElements)
		{
			d3dElement = GfxInputElementToD3D11(*gfxElement++);
		}

		HRESULT hr = d3dDevice->CreateInputLayout(d3dElements.data(), count, shader.GetByteCode(), shader.GetSizeInBytes(), this->d3dInputLayout);
		if (FAILED(hr))
			return ResultError(Failure, "GfxInputLayoutD3D11: failed to create input layout");

		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxPipelineStateD3D11
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GfxPipelineStateD3D11::GfxPipelineStateD3D11(GfxSystem &gfxSystem) :
		GfxPipelineState(gfxSystem)
	{
		memset(this->hashSamplerState, 0, sizeof(this->hashSamplerState));
		this->hashRasterizerState = 0;
		this->hashDepthStencilState = 0;
		this->hashBlendState = 0;
	}

	GfxPipelineStateD3D11::~GfxPipelineStateD3D11()
	{
	}

	Result GfxPipelineStateD3D11::OnSetRenderState(const GfxRenderState &renderState)
	{
		GfxSystemD3D11 &d3dSystem = static_cast<GfxSystemD3D11&>(this->GetGfxSystem());
		ID3D11Device *d3dDevice = d3dSystem.GetDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxPipelineStateD3D11: failed to initialize, device not ready");

		// SamplerState
		for (ur_uint i = 0; i < GfxRenderState::MaxSamplerStates; ++i)
		{
			ur_size hash = ComputeHash((const ur_byte*)&renderState.SamplerState[i], sizeof(renderState.SamplerState[i]));
			if (this->hashSamplerState[i] != hash)
			{
				this->hashSamplerState[i] = hash;
				this->d3dSamplerState[i].reset(ur_null);
				D3D11_SAMPLER_DESC desc = GfxSamplerStateToD3D11(renderState.SamplerState[i]);
				HRESULT hr = d3dDevice->CreateSamplerState(&desc, this->d3dSamplerState[i]);
				if (FAILED(hr))
					return ResultError(Failure, "GfxPipelineStateD3D11: failed to create sampler state object");
			}
		}

		// RasterizerState
		ur_size hash = ComputeHash((const ur_byte*)&renderState.RasterizerState, sizeof(renderState.RasterizerState));
		if (this->hashRasterizerState != hash)
		{
			this->hashRasterizerState = hash;
			this->d3dRasterizerState.reset(ur_null);
			D3D11_RASTERIZER_DESC desc = GfxRasterizerStateToD3D11(renderState.RasterizerState);
			HRESULT hr = d3dDevice->CreateRasterizerState(&desc, this->d3dRasterizerState);
			if (FAILED(hr))
				return ResultError(Failure, "GfxPipelineStateD3D11: failed to create rasterizer state object");
		}

		// DepthStencilState
		hash = ComputeHash((const ur_byte*)&renderState.DepthStencilState, sizeof(renderState.DepthStencilState));
		if (this->hashDepthStencilState != hash)
		{
			this->hashDepthStencilState = hash;
			this->d3dDepthStencilState.reset(ur_null);
			D3D11_DEPTH_STENCIL_DESC desc = GfxDepthStencilStateToD3D11(renderState.DepthStencilState);
			HRESULT hr = d3dDevice->CreateDepthStencilState(&desc, this->d3dDepthStencilState);
			if (FAILED(hr))
				return ResultError(Failure, "GfxPipelineStateD3D11: failed to create depth stencil state object");
		}

		// BlendState
		hash = ComputeHash((const ur_byte*)&renderState.BlendState, sizeof(renderState.BlendState));
		if (this->hashBlendState != hash)
		{
			this->hashBlendState = hash;
			this->d3dBlendState.reset(ur_null);
			D3D11_BLEND_DESC desc;
			desc.AlphaToCoverageEnable = false;
			desc.IndependentBlendEnable = true;
			for (ur_uint irt = 0; irt < GfxRenderState::MaxRenderTargets; ++irt)
			{
				desc.RenderTarget[irt] = GfxBlendStateToD3D11(renderState.BlendState[irt]);
			}
			HRESULT hr = d3dDevice->CreateBlendState(&desc, this->d3dBlendState);
			if (FAILED(hr))
				return ResultError(Failure, "GfxPipelineStateD3D11: failed to create blend state object");
		}

		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxSamplerD3D11
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	
	GfxSamplerD3D11::GfxSamplerD3D11(GfxSystem &gfxSystem) :
		GfxSampler(gfxSystem)
	{
	}

	GfxSamplerD3D11::~GfxSamplerD3D11()
	{
	}

	Result GfxSamplerD3D11::OnInitialize(const GfxSamplerState& state)
	{
		this->d3dSamplerState.reset(ur_null);

		GfxSystemD3D11 &d3dSystem = static_cast<GfxSystemD3D11&>(this->GetGfxSystem());
		ID3D11Device *d3dDevice = d3dSystem.GetDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxSamplerD3D11: failed to initialize, device not ready");

		D3D11_SAMPLER_DESC desc = GfxSamplerStateToD3D11(state);
		HRESULT hr = d3dDevice->CreateSamplerState(&desc, this->d3dSamplerState);
		if (FAILED(hr))
			return ResultError(Failure, "GfxSamplerD3D11: failed to create sampler state object");

		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxPipelineStateObjectD3D11
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxPipelineStateObjectD3D11::GfxPipelineStateObjectD3D11(GfxSystem &gfxSystem) :
		GfxPipelineStateObject(gfxSystem)
	{

	}

	GfxPipelineStateObjectD3D11::~GfxPipelineStateObjectD3D11()
	{

	}

	Result GfxPipelineStateObjectD3D11::OnInitialize(const StateFlags& changedStates)
	{
		GfxSystemD3D11 &d3dSystem = static_cast<GfxSystemD3D11&>(this->GetGfxSystem());
		ID3D11Device *d3dDevice = d3dSystem.GetDevice();
		if (ur_null == d3dDevice)
			return ResultError(NotInitialized, "GfxPipelineStateObjectD3D11: failed to initialize, device not ready");
		
		if (InputLayoutFlag & changedStates)
		{
			this->d3dInputLayout.reset(ur_null);
			auto& gfxElements = this->GetInputLayout()->GetElements();
			std::vector<D3D11_INPUT_ELEMENT_DESC> d3dElements(gfxElements.size());
			const GfxInputElement *gfxElement = gfxElements.data();
			for (auto &d3dElement : d3dElements)
			{
				d3dElement = GfxInputElementToD3D11(*gfxElement++);
			}
			HRESULT hr = d3dDevice->CreateInputLayout(d3dElements.data(), UINT(gfxElements.size()),
				this->GetVertexShader()->GetByteCode(), this->GetVertexShader()->GetSizeInBytes(), this->d3dInputLayout);
			if (FAILED(hr))
				return ResultError(Failure, "GfxPipelineStateObjectD3D11: failed to create input layout");
		}
		if (RasterizerStateFlag & changedStates)
		{
			this->d3dRasterizerState.reset(ur_null);
			D3D11_RASTERIZER_DESC desc = GfxRasterizerStateToD3D11(this->GetRasterizerState());
			HRESULT hr = d3dDevice->CreateRasterizerState(&desc, this->d3dRasterizerState);
			if (FAILED(hr))
				return ResultError(Failure, "GfxPipelineStateObjectD3D11: failed to create rasterizer state object");
		}
		if (DepthStencilStateFlag & changedStates)
		{
			this->d3dDepthStencilState.reset(ur_null);
			D3D11_DEPTH_STENCIL_DESC desc = GfxDepthStencilStateToD3D11(this->GetDepthStencilState());
			HRESULT hr = d3dDevice->CreateDepthStencilState(&desc, this->d3dDepthStencilState);
			if (FAILED(hr))
				return ResultError(Failure, "GfxPipelineStateObjectD3D11: failed to create depth stencil state object");
		}
		if (BlendStateFlag & changedStates)
		{
			this->d3dBlendState.reset(ur_null);
			D3D11_BLEND_DESC desc;
			desc.AlphaToCoverageEnable = false;
			desc.IndependentBlendEnable = true;
			for (ur_uint irt = 0; irt < GfxRenderState::MaxRenderTargets; ++irt)
			{
				desc.RenderTarget[irt] = GfxBlendStateToD3D11(this->GetBlendState(irt));
			}
			HRESULT hr = d3dDevice->CreateBlendState(&desc, this->d3dBlendState);
			if (FAILED(hr))
				return ResultError(Failure, "GfxPipelineStateObjectD3D11: failed to create blend state object");
		}

		return Result(Success);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GfxResourceBindingD3D11
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	GfxResourceBindingD3D11::GfxResourceBindingD3D11(GfxSystem &gfxSystem) :
		GfxResourceBinding(gfxSystem)
	{

	}

	GfxResourceBindingD3D11::~GfxResourceBindingD3D11()
	{

	}

	Result GfxResourceBindingD3D11::OnInitialize()
	{
		// TODO:
		return Result(NotImplemented);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Utilities
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	D3D11_USAGE GfxUsageToD3D11(GfxUsage usage)
	{
		switch (usage)
		{
		case GfxUsage::Default: return D3D11_USAGE_DEFAULT;
		case GfxUsage::Immutable: return D3D11_USAGE_IMMUTABLE;
		case GfxUsage::Dynamic: return D3D11_USAGE_DYNAMIC;
		case GfxUsage::Readback: return D3D11_USAGE_STAGING;
		}
		return D3D11_USAGE(-1);
	}

	D3D11_BIND_FLAG GfxBindFlagToD3D11(GfxBindFlag flag)
	{
		return (D3D11_BIND_FLAG)flag;
	}

	UINT GfxBindFlagsToD3D11(ur_uint flags)
	{
		return (UINT)flags;
	}

	D3D11_CPU_ACCESS_FLAG GfxAccessFlagToD3D11_CPUAccessFlag(GfxAccessFlag flag)
	{
		switch (flag)
		{
		case GfxAccessFlag::Read: return D3D11_CPU_ACCESS_READ;
		case GfxAccessFlag::Write: return D3D11_CPU_ACCESS_WRITE;
		}
		return (D3D11_CPU_ACCESS_FLAG)0;
	}

	UINT GfxAccessFlagsToD3D11_CPUAccessFlags(ur_uint flags)
	{
		UINT d3dFlags = 0;
		if (flags & (ur_uint)GfxAccessFlag::Write) d3dFlags |= D3D11_CPU_ACCESS_WRITE;
		if (flags & (ur_uint)GfxAccessFlag::Read) d3dFlags |= D3D11_CPU_ACCESS_READ;
		return (UINT)d3dFlags;
	}

	D3D11_MAP GfxGPUAccessFlagToD3D11(GfxGPUAccess gpuAccess)
	{
		switch (gpuAccess)
		{
		case GfxGPUAccess::Read: return D3D11_MAP_READ;
		case GfxGPUAccess::Write: return D3D11_MAP_WRITE;
		case GfxGPUAccess::ReadWrite: return D3D11_MAP_READ_WRITE;
		case GfxGPUAccess::WriteDiscard: return D3D11_MAP_WRITE_DISCARD;
		case GfxGPUAccess::WriteNoOverwrite: return D3D11_MAP_WRITE_NO_OVERWRITE;
		}
		return (D3D11_MAP)0;
	}

	D3D11_VIEWPORT GfxViewPortToD3D11(const GfxViewPort &viewPort)
	{
		D3D11_VIEWPORT d3dViewPort;
		d3dViewPort.TopLeftX = viewPort.X;
		d3dViewPort.TopLeftY = viewPort.Y;
		d3dViewPort.Width = viewPort.Width;
		d3dViewPort.Height = viewPort.Height;
		d3dViewPort.MinDepth = viewPort.DepthMin;
		d3dViewPort.MaxDepth = viewPort.DepthMax;
		return d3dViewPort;
	}

	GfxViewPort GfxViewPortFromD3D11(const D3D11_VIEWPORT &viewPort)
	{
		GfxViewPort gfxViewPort;
		gfxViewPort.X = viewPort.TopLeftX;
		gfxViewPort.Y = viewPort.TopLeftY;
		gfxViewPort.Width = viewPort.Width;
		gfxViewPort.Height = viewPort.Height;
		gfxViewPort.DepthMin = viewPort.MinDepth;
		gfxViewPort.DepthMax = viewPort.MaxDepth;
		return gfxViewPort;
	}

	D3D11_RECT RectIToD3D11(const RectI &rect)
	{
		D3D11_RECT d3dRect;
		d3dRect.left = (LONG)rect.Min.x;
		d3dRect.right = (LONG)rect.Max.x;
		d3dRect.top = (LONG)rect.Min.y;
		d3dRect.bottom = (LONG)rect.Max.y;
		return d3dRect;
	}

	D3D11_TEXTURE2D_DESC GfxTextureDescToD3D11(const GfxTextureDesc &desc)
	{
		D3D11_TEXTURE2D_DESC d3dDesc;
		d3dDesc.Width = desc.Width;
		d3dDesc.Height = desc.Height;
		d3dDesc.MipLevels = desc.Levels;
		d3dDesc.ArraySize = 1;
		d3dDesc.Format = GfxFormatToDXGI(desc.Format, desc.FormatView);
		d3dDesc.SampleDesc.Count = 1;
		d3dDesc.SampleDesc.Quality = 0;
		d3dDesc.Usage = GfxUsageToD3D11(desc.Usage);
		d3dDesc.BindFlags = GfxBindFlagsToD3D11(desc.BindFlags);
		d3dDesc.CPUAccessFlags = GfxAccessFlagsToD3D11_CPUAccessFlags(desc.AccessFlags);
		d3dDesc.MiscFlags = 0;
		return d3dDesc;
	}

	D3D11_BUFFER_DESC GfxBufferDescToD3D11(const GfxBufferDesc &desc)
	{
		D3D11_BUFFER_DESC d3dDesc;
		d3dDesc.ByteWidth = desc.Size;
		d3dDesc.Usage = GfxUsageToD3D11(desc.Usage);
		d3dDesc.BindFlags = GfxBindFlagsToD3D11(desc.BindFlags);
		d3dDesc.CPUAccessFlags = GfxAccessFlagsToD3D11_CPUAccessFlags(desc.AccessFlags);
		d3dDesc.MiscFlags = 0;
		d3dDesc.StructureByteStride = 0; // use ElementSize here if buffer is a StructuredBuffer
		return d3dDesc;
	}

	LPCSTR GfxSemanticToD3D11(GfxSemantic semantic)
	{
		switch (semantic)
		{
		case GfxSemantic::Position: return "POSITION";
		case GfxSemantic::TexCoord: return "TEXCOORD";
		case GfxSemantic::Color: return "COLOR";
		case GfxSemantic::Tangtent: return "TANGENT";
		case GfxSemantic::Normal: return "NORMAL";
		case GfxSemantic::Binormal: return "BINORMAL";
		}
		return "UNKNOWN";
	}

	D3D11_INPUT_ELEMENT_DESC GfxInputElementToD3D11(const GfxInputElement &element)
	{
		D3D11_INPUT_ELEMENT_DESC d3dElement;
		d3dElement.SemanticName = GfxSemanticToD3D11(element.Semantic);
		d3dElement.SemanticIndex = element.SemanticIdx;
		d3dElement.Format = GfxFormatToDXGI(element.Format, element.FormatView);
		d3dElement.InputSlot = element.SlotIdx;
		d3dElement.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		d3dElement.InputSlotClass = (element.InstanceStepRate > 0 ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA);
		d3dElement.InstanceDataStepRate = element.InstanceStepRate;
		return d3dElement;
	}

	D3D11_PRIMITIVE_TOPOLOGY GfxPrimitiveTopologyToD3D11(GfxPrimitiveTopology topology)
	{
		switch (topology)
		{
		case GfxPrimitiveTopology::PointList: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		case GfxPrimitiveTopology::LineList: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case GfxPrimitiveTopology::LineStrip: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
		case GfxPrimitiveTopology::TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case GfxPrimitiveTopology::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		}
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}

	D3D11_RENDER_TARGET_BLEND_DESC GfxBlendStateToD3D11(const GfxBlendState &state)
	{
		D3D11_RENDER_TARGET_BLEND_DESC desc;
		desc.BlendEnable = state.BlendEnable;
		desc.SrcBlend = GfxBlendFactorToD3D11(state.SrcBlend);
		desc.DestBlend = GfxBlendFactorToD3D11(state.DstBlend);
		desc.BlendOp = GfxBlendOpToD3D11(state.BlendOp);
		desc.SrcBlendAlpha = GfxBlendFactorToD3D11(state.SrcBlendAlpha);
		desc.DestBlendAlpha = GfxBlendFactorToD3D11(state.DstBlendAlpha);
		desc.BlendOpAlpha = GfxBlendOpToD3D11(state.BlendOpAlpha);
		desc.RenderTargetWriteMask = state.RenderTargetWriteMask;
		return desc;
	}

	D3D11_BLEND GfxBlendFactorToD3D11(GfxBlendFactor blendFactor)
	{
		switch (blendFactor)
		{
		case GfxBlendFactor::Zero: return D3D11_BLEND_ZERO;
		case GfxBlendFactor::One: return D3D11_BLEND_ONE;
		case GfxBlendFactor::SrcColor: return D3D11_BLEND_SRC_COLOR;
		case GfxBlendFactor::InvSrcColor: return D3D11_BLEND_INV_SRC_COLOR;
		case GfxBlendFactor::SrcAlpha: return D3D11_BLEND_SRC_ALPHA;
		case GfxBlendFactor::InvSrcAlpha: return D3D11_BLEND_INV_SRC_ALPHA;
		case GfxBlendFactor::DstAlpha: return D3D11_BLEND_DEST_ALPHA;
		case GfxBlendFactor::InvDstAlpha: return D3D11_BLEND_INV_DEST_ALPHA;
		case GfxBlendFactor::DstColor: return D3D11_BLEND_DEST_COLOR;
		case GfxBlendFactor::InvDstColor: return D3D11_BLEND_INV_DEST_COLOR;
		}
		return D3D11_BLEND(0);
	}

	D3D11_BLEND_OP GfxBlendOpToD3D11(GfxBlendOp blendOp)
	{
		switch (blendOp)
		{
		case GfxBlendOp::Add: return D3D11_BLEND_OP_ADD;
		case GfxBlendOp::Subtract: return D3D11_BLEND_OP_SUBTRACT;
		case GfxBlendOp::RevSubtract: return D3D11_BLEND_OP_REV_SUBTRACT;
		case GfxBlendOp::Min: return D3D11_BLEND_OP_MIN;
		case GfxBlendOp::Max: return D3D11_BLEND_OP_MAX;
		}
		return D3D11_BLEND_OP(0);
	}

	D3D11_COMPARISON_FUNC GfxCmpFuncToD3D11(GfxCmpFunc func)
	{
		switch (func)
		{
		case GfxCmpFunc::Never: return D3D11_COMPARISON_NEVER;
		case GfxCmpFunc::Less: return D3D11_COMPARISON_LESS;
		case GfxCmpFunc::Equal: return D3D11_COMPARISON_EQUAL;
		case GfxCmpFunc::LessEqual: return D3D11_COMPARISON_LESS_EQUAL;
		case GfxCmpFunc::Greater: return D3D11_COMPARISON_GREATER;
		case GfxCmpFunc::NotEqual: return D3D11_COMPARISON_NOT_EQUAL;
		case GfxCmpFunc::GreaterEqual: return D3D11_COMPARISON_GREATER_EQUAL;
		case GfxCmpFunc::Always: return D3D11_COMPARISON_ALWAYS;
		}
		return (D3D11_COMPARISON_FUNC)0;
	}

	D3D11_FILTER GfxFilterToD3D11(GfxFilter minFilter, GfxFilter magFilter, GfxFilter mipFilter)
	{
		if (GfxFilter::Point == minFilter && GfxFilter::Point == magFilter && GfxFilter::Point == magFilter) return D3D11_FILTER_MIN_MAG_MIP_POINT;
		if (GfxFilter::Point == minFilter && GfxFilter::Point == magFilter && GfxFilter::Linear == magFilter) return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
		if (GfxFilter::Point == minFilter && GfxFilter::Linear == magFilter && GfxFilter::Point == magFilter) return D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
		if (GfxFilter::Point == minFilter && GfxFilter::Linear == magFilter && GfxFilter::Linear == magFilter) return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
		if (GfxFilter::Linear == minFilter && GfxFilter::Point == magFilter && GfxFilter::Point == magFilter) return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
		if (GfxFilter::Linear == minFilter && GfxFilter::Point == magFilter && GfxFilter::Linear == magFilter) return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		if (GfxFilter::Linear == minFilter && GfxFilter::Linear == magFilter && GfxFilter::Point == magFilter) return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		if (GfxFilter::Linear == minFilter && GfxFilter::Linear == magFilter && GfxFilter::Linear == magFilter) return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		if (GfxFilter::Anisotropic == minFilter || GfxFilter::Anisotropic == magFilter || GfxFilter::Anisotropic == magFilter) return D3D11_FILTER_ANISOTROPIC;
		return (D3D11_FILTER)0;
	}

	D3D11_TEXTURE_ADDRESS_MODE GfxTextureAddressModeToD3D11(GfxTextureAddressMode mode)
	{
		switch (mode)
		{
		case GfxTextureAddressMode::Wrap: return D3D11_TEXTURE_ADDRESS_WRAP;
		case GfxTextureAddressMode::Mirror: return D3D11_TEXTURE_ADDRESS_MIRROR;
		case GfxTextureAddressMode::Clamp: return D3D11_TEXTURE_ADDRESS_CLAMP;
		case GfxTextureAddressMode::Border: return D3D11_TEXTURE_ADDRESS_BORDER;
		case GfxTextureAddressMode::MirrorOnce: return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
		}
		return (D3D11_TEXTURE_ADDRESS_MODE)0;
	}

	D3D11_SAMPLER_DESC GfxSamplerStateToD3D11(const GfxSamplerState &state)
	{
		D3D11_SAMPLER_DESC desc;
		desc.Filter = GfxFilterToD3D11(state.MinFilter, state.MagFilter, state.MipFilter);
		desc.AddressU = GfxTextureAddressModeToD3D11(state.AddressU);
		desc.AddressV = GfxTextureAddressModeToD3D11(state.AddressV);
		desc.AddressW = GfxTextureAddressModeToD3D11(state.AddressW);
		desc.MipLODBias = (FLOAT)state.MipLodBias;
		desc.MaxAnisotropy = (UINT)state.MaxAnisotropy;
		desc.ComparisonFunc = GfxCmpFuncToD3D11(state.CmpFunc);
		desc.BorderColor[0] = (FLOAT)state.BorderColor.x;
		desc.BorderColor[1] = (FLOAT)state.BorderColor.y;
		desc.BorderColor[2] = (FLOAT)state.BorderColor.z;
		desc.BorderColor[3] = (FLOAT)state.BorderColor.w;
		desc.MinLOD = (FLOAT)state.MipLodMin;
		desc.MaxLOD = (FLOAT)state.MipLodMax;
		return desc;
	}

	D3D11_FILL_MODE GfxFillModeToD3D11(GfxFillMode mode)
	{
		switch (mode)
		{
		case GfxFillMode::Wireframe: return D3D11_FILL_WIREFRAME;
		case GfxFillMode::Solid: return D3D11_FILL_SOLID;
		}
		return (D3D11_FILL_MODE)0;
	}

	D3D11_CULL_MODE GfxCullModeToD3D11(GfxCullMode mode)
	{
		switch (mode)
		{
		case GfxCullMode::None: return D3D11_CULL_NONE;
		case GfxCullMode::CW: return D3D11_CULL_FRONT;
		case GfxCullMode::CCW: return D3D11_CULL_BACK;
		}
		return (D3D11_CULL_MODE)0;
	}

	D3D11_RASTERIZER_DESC GfxRasterizerStateToD3D11(const GfxRasterizerState& state)
	{
		D3D11_RASTERIZER_DESC desc;
		desc.FillMode = GfxFillModeToD3D11(state.FillMode);
		desc.CullMode = GfxCullModeToD3D11(state.CullMode);
		desc.FrontCounterClockwise = FALSE;
		desc.DepthBias = (INT)state.DepthBias;
		desc.DepthBiasClamp = (FLOAT)state.DepthBiasClamp;
		desc.SlopeScaledDepthBias = (FLOAT)state.SlopeScaledDepthBias;
		desc.DepthClipEnable = (BOOL)state.DepthClipEnable;
		desc.ScissorEnable = (BOOL)state.ScissorEnable;
		desc.MultisampleEnable = (BOOL)state.MultisampleEnable;
		desc.AntialiasedLineEnable = FALSE;
		return desc;
	}

	D3D11_STENCIL_OP GfxStencilOpToD3D11(GfxStencilOp stencilOp)
	{
		switch (stencilOp)
		{
		case GfxStencilOp::Keep: return D3D11_STENCIL_OP_KEEP;
		case GfxStencilOp::Zero: return D3D11_STENCIL_OP_ZERO;
		case GfxStencilOp::Replace: return D3D11_STENCIL_OP_REPLACE;
		case GfxStencilOp::IncrSat: return D3D11_STENCIL_OP_INCR_SAT;
		case GfxStencilOp::DecrSat: return D3D11_STENCIL_OP_DECR_SAT;
		case GfxStencilOp::Invert: return D3D11_STENCIL_OP_INVERT;
		case GfxStencilOp::Incr: return D3D11_STENCIL_OP_INCR;
		case GfxStencilOp::Decr: return D3D11_STENCIL_OP_DECR;
		}
		return (D3D11_STENCIL_OP)0;
	}

	D3D11_DEPTH_STENCILOP_DESC GfxDepthStencilOpDescToD3D11(const GfxDepthStencilOpDesc& gfxDesc)
	{
		D3D11_DEPTH_STENCILOP_DESC desc;
		desc.StencilFailOp = GfxStencilOpToD3D11(gfxDesc.StencilFailOp);
		desc.StencilDepthFailOp = GfxStencilOpToD3D11(gfxDesc.StencilDepthFailOp);
		desc.StencilPassOp = GfxStencilOpToD3D11(gfxDesc.StencilPassOp);
		desc.StencilFunc = GfxCmpFuncToD3D11(gfxDesc.StencilFunc);
		return desc;
	}

	D3D11_DEPTH_STENCIL_DESC GfxDepthStencilStateToD3D11(const GfxDepthStencilState &state)
	{
		D3D11_DEPTH_STENCIL_DESC desc;
		desc.DepthEnable = (BOOL)state.DepthEnable;
		desc.DepthWriteMask = (state.DepthWriteEnable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO);
		desc.DepthFunc = GfxCmpFuncToD3D11(state.DepthFunc);
		desc.StencilEnable = (BOOL)state.StencilEnable;
		desc.StencilReadMask = (UINT8)state.StencilReadMask;
		desc.StencilWriteMask = (UINT8)state.StencilWriteMask;
		desc.FrontFace = GfxDepthStencilOpDescToD3D11(state.FrontFace);
		desc.BackFace = GfxDepthStencilOpDescToD3D11(state.BackFace);
		return desc;
	}

} // end namespace UnlimRealms