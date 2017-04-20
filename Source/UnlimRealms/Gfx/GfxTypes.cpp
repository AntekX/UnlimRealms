///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Gfx/GfxTypes.h"

namespace UnlimRealms
{

	const GfxBlendState GfxBlendState::Default = {
		false,							// BlendEnable
		GfxBlendFactor::One,			// SrcBlend
		GfxBlendFactor::Zero,			// DstBlend
		GfxBlendFactor::One,			// SrcBlendAlpha
		GfxBlendFactor::Zero,			// DstBlendAlpha
		GfxBlendOp::Add,				// BlendOp
		GfxBlendOp::Add,				// BlendOpAlpha
		ur_byte(GfxChannelFlag::All)	// RenderTargetWriteMask
	};

	const GfxDepthStencilOpDesc GfxDepthStencilOpDesc::Default = {
		GfxStencilOp::Keep,				// StencilFailOp
		GfxStencilOp::Keep,				// StencilDepthFailOp
		GfxStencilOp::Keep,				// StencilPassOp
		GfxCmpFunc::Always				// StencilFunc
	};

	const ur_byte GfxDepthStencilState::DefaultStencilReadMask = 0xff;
	
	const ur_byte GfxDepthStencilState::DefaultStencilWriteMask = 0xff;

	const GfxDepthStencilState GfxDepthStencilState::Default = {
		true,							// DepthEnable
		true,							// DepthWriteEnable
		GfxCmpFunc::Less,				// DepthFunc
		false,							// StencilEnable
		DefaultStencilReadMask,			// StencilReadMask
		DefaultStencilWriteMask,		// StencilWriteMask
		GfxDepthStencilOpDesc::Default,	// FrontFace
		GfxDepthStencilOpDesc::Default	// BackFace
	};

	const GfxSamplerState GfxSamplerState::Default = {
		GfxFilter::Linear,				// MinFilter
		GfxFilter::Linear,				// MagFilter
		GfxFilter::Linear,				// MipFilter
		GfxTextureAddressMode::Clamp,	// AddressU
		GfxTextureAddressMode::Clamp,	// AddressV
		GfxTextureAddressMode::Clamp,	// AddressW
		0.0f,							// MipLodBias
		-std::numeric_limits<ur_float>::max(),	// MipLodMin
		+std::numeric_limits<ur_float>::max(),	// MipLodMax
		1,								// MaxAnisotropy;
		ur_float4(1.0f),				// BorderColor
		GfxCmpFunc::Never				// CmpFunc
	};

	const GfxRasterizerState GfxRasterizerState::Default = {
		GfxFillMode::Solid,				// FillMode
		GfxCullMode::CCW,				// CullMode
		0,								// DepthBias
		0.0f,							// DepthBiasClamp
		0.0f,							// SlopeScaledDepthBias
		true,							// DepthClipEnable
		false,							// ScissorEnable
		false							// MultisampleEnable
	};

	const GfxRenderState GfxRenderState::Default = {
		{									// SamplerState
			GfxSamplerState::Default,
			GfxSamplerState::Default,
			GfxSamplerState::Default,
			GfxSamplerState::Default,
			GfxSamplerState::Default,
			GfxSamplerState::Default,
			GfxSamplerState::Default,
			GfxSamplerState::Default,
			GfxSamplerState::Default,
			GfxSamplerState::Default,
			GfxSamplerState::Default,
			GfxSamplerState::Default,
			GfxSamplerState::Default,
			GfxSamplerState::Default,
			GfxSamplerState::Default,
			GfxSamplerState::Default
		},
		GfxRasterizerState::Default,		// RasterizerState
		GfxDepthStencilState::Default,		// DepthStencilState
		{									// BlendState
			GfxBlendState::Default,
			GfxBlendState::Default,
			GfxBlendState::Default,
			GfxBlendState::Default,
			GfxBlendState::Default,
			GfxBlendState::Default,
			GfxBlendState::Default,
			GfxBlendState::Default
		},
	};

} // end namespace UnlimRealms