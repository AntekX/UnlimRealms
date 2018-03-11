///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{

	inline WinCanvas* GfxSystemD3D11::GetWinCanvas() const
	{
		return this->winCanvas;
	}

	inline IDXGIFactory1* GfxSystemD3D11::GetDXGIFactory() const
	{
		return this->dxgiFactory;
	}
	
	inline ID3D11Device* GfxSystemD3D11::GetDevice() const
	{
		return this->d3dDevice;
	}

	inline ID3D11DeviceContext* GfxSystemD3D11::GetDeviceContext() const
	{
		return this->d3dImmediateContext;
	}

	inline void GfxSystemD3D11::AddCommandList(shared_ref<ID3D11CommandList> &d3dCommandList)
	{
		std::lock_guard<std::mutex> lock(commandListMutex);

		d3dCommandLists[this->commandListId].push_back(d3dCommandList);
	}

	inline ID3D11Texture2D* GfxTextureD3D11::GetD3DTexture() const
	{
		return this->d3dTexture.get();
	}

	inline ID3D11ShaderResourceView* GfxTextureD3D11::GetSRV() const
	{
		return this->d3dSRV.get();
	}

	inline ID3D11RenderTargetView* GfxRenderTargetD3D11::GetRTView() const
	{
		return this->d3dRTView.get();
	}

	inline ID3D11DepthStencilView* GfxRenderTargetD3D11::GetDSView() const
	{
		return this->d3dDSView.get();
	}

	inline ID3D11Buffer* GfxBufferD3D11::GetD3DBuffer() const
	{
		return this->d3dBuffer.get();
	}

	inline ID3D11VertexShader* GfxVertexShaderD3D11::GetD3DShader() const
	{
		return this->d3dVertexShader.get();
	}

	inline ID3D11PixelShader* GfxPixelShaderD3D11::GetD3DShader() const
	{
		return this->d3dPixelShader.get();
	}

	inline ID3D11InputLayout* GfxInputLayoutD3D11::GetD3DInputLayout() const
	{
		return this->d3dInputLayout.get();
	}

	inline ID3D11SamplerState* GfxPipelineStateD3D11::GetD3DSamplerState(ur_uint idx) const
	{
		return (idx < GfxRenderState::MaxSamplerStates ? this->d3dSamplerState[idx].get() : ur_null);
	}

	inline ID3D11RasterizerState* GfxPipelineStateD3D11::GetD3DRasterizerState() const
	{
		return this->d3dRasterizerState.get();
	}

	inline ID3D11DepthStencilState* GfxPipelineStateD3D11::GetD3DDepthStencilState() const
	{
		return this->d3dDepthStencilState.get();
	}

	inline ID3D11BlendState* GfxPipelineStateD3D11::GetD3DBlendState() const
	{
		return this->d3dBlendState.get();
	}

} // end namespace UnlimRealms