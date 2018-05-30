///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Core/Core.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx enumerations
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	enum class UR_DECL GfxFormat : ur_byte
	{
		Unknown = 0,
		R32G32B32A32,
		R32G32B32,
		R16G16B16A16,
		R32G32,
		R32G8X24,
		R8G8B8A8,
		B8G8R8A8,
		B8G8R8X8,
		R16G16,
		R32,
		R24G8,
		R8G8,
		B5G6R5,
		B5G5R5A1,
		R16,
		R8,
		A8,
		R1,
		BC1,
		BC2,
		BC3,
		BC4,
		BC5,
		BC6H,
		BC7,
		Count
	};

	enum class UR_DECL GfxFormatView : ur_byte
	{
		Typeless,
		Unorm,
		Snorm,
		Uint,
		Sint,
		Float,
		Count
	};

	enum class UR_DECL GfxUsage : ur_byte
	{
		Default,
		Immutable,
		Dynamic,
		Readback
	};

	enum class UR_DECL GfxBindFlag : ur_byte
	{
		VertexBuffer = (1 << 0),
		IndexBuffer = (1 << 1),
		ConstantBuffer = (1 << 2),
		ShaderResource = (1 << 3),
		StreamOutput = (1 << 4),
		RenderTarget = (1 << 5),
		DepthStencil = (1 << 6),
		UnorderedAccess = (1 << 7)
	};

	enum class UR_DECL GfxAccessFlag : ur_byte
	{
		Read = (1 << 0),
		Write = (1 << 1)
	};

	enum class UR_DECL GfxGPUAccess : ur_byte
	{
		Read,
		Write,
		ReadWrite,
		WriteDiscard,
		WriteNoOverwrite
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Render view port
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	struct UR_DECL GfxViewPort
	{
		ur_float X;
		ur_float Y;
		ur_float Width;
		ur_float Height;
		ur_float DepthMin;
		ur_float DepthMax;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Texture 2D description
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	struct UR_DECL GfxTextureDesc
	{
		ur_uint Width;
		ur_uint Height;
		ur_uint Levels;
		GfxFormat Format;
		GfxFormatView FormatView;
		GfxUsage Usage;
		ur_uint BindFlags;
		ur_uint AccessFlags;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Presentation parameters
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	struct UR_DECL GfxPresentParams
	{
		ur_uint BufferWidth;
		ur_uint BufferHeight;
		ur_uint BufferCount;
		GfxFormat BufferFormat;
		ur_bool Fullscreen;
		ur_uint FullscreenRefreshRate;
		ur_bool DepthStencilEnabled;
		GfxFormat DepthStencilFormat;
		ur_uint MutisampleCount;
		ur_uint MutisampleQuality;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx resource data
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	struct UR_DECL GfxResourceData
	{
		void *Ptr;
		ur_uint RowPitch;
		ur_uint SlicePitch;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Buffer description
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	struct UR_DECL GfxBufferDesc
	{
		ur_uint Size;
		ur_uint ElementSize;
		GfxUsage Usage;
		ur_uint BindFlags;
		ur_uint AccessFlags;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx primitive topology type
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GfxPrimitiveTopology : ur_byte
	{
		Undefined,
		PointList,
		LineList,
		LineStrip,
		TriangleList,
		TriangleStrip
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx color channel flags
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GfxChannelFlag : ur_byte
	{
		None = 0,
		Red = 1,
		Green = 2,
		Blue = 4,
		Alpha = 8,
		All = (Red | Green | Blue | Alpha)
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx blend factor types
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GfxBlendFactor : ur_byte
	{
		Zero,
		One,
		SrcColor,
		InvSrcColor,
		SrcAlpha,
		InvSrcAlpha,
		DstAlpha,
		InvDstAlpha,
		DstColor,
		InvDstColor
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx blend operation types
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GfxBlendOp : ur_byte
	{
		Add,
		Subtract,
		RevSubtract,
		Min,
		Max
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx blend state description
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GfxBlendState
	{
		ur_bool BlendEnable;
		GfxBlendFactor SrcBlend;
		GfxBlendFactor DstBlend;
		GfxBlendFactor SrcBlendAlpha;
		GfxBlendFactor DstBlendAlpha;
		GfxBlendOp BlendOp;
		GfxBlendOp BlendOpAlpha;
		ur_byte RenderTargetWriteMask;

		static const GfxBlendState Default;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx comparison function types
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GfxCmpFunc : ur_byte
	{
		Never,
		Less,
		Equal,
		LessEqual,
		Greater,
		NotEqual,
		GreaterEqual,
		Always
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx stencil operation types
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GfxStencilOp : ur_byte
	{
		Keep,
		Zero,
		Replace,
		IncrSat,
		DecrSat,
		Invert,
		Incr,
		Decr
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx depth stencil operations description
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GfxDepthStencilOpDesc
	{
		GfxStencilOp StencilFailOp;
		GfxStencilOp StencilDepthFailOp;
		GfxStencilOp StencilPassOp;
		GfxCmpFunc StencilFunc;

		static const GfxDepthStencilOpDesc Default;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx depth stencil state description
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GfxDepthStencilState
	{
		ur_bool DepthEnable;
		ur_bool DepthWriteEnable;
		GfxCmpFunc DepthFunc;
		ur_bool StencilEnable;
		ur_byte StencilReadMask;
		ur_byte StencilWriteMask;
		GfxDepthStencilOpDesc FrontFace;
		GfxDepthStencilOpDesc BackFace;

		static const ur_byte DefaultStencilReadMask;
		static const ur_byte DefaultStencilWriteMask;
		static const GfxDepthStencilState Default;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx filter types
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GfxFilter : ur_byte
	{
		Point,
		Linear,
		Anisotropic
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx texture coordinates resolving mode
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GfxTextureAddressMode : ur_byte
	{
		Wrap,
		Mirror,
		Clamp,
		Border,
		MirrorOnce
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx sampler state description
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GfxSamplerState
	{
		GfxFilter MinFilter;
		GfxFilter MagFilter;
		GfxFilter MipFilter;
		GfxTextureAddressMode AddressU;
		GfxTextureAddressMode AddressV;
		GfxTextureAddressMode AddressW;
		ur_float MipLodBias;
		ur_float MipLodMin;
		ur_float MipLodMax;
		ur_uint MaxAnisotropy;
		ur_float4 BorderColor;
		GfxCmpFunc CmpFunc;

		static const GfxSamplerState Default;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx primitives fill mode
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GfxFillMode : ur_byte
	{
		Wireframe,
		Solid
	};

	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx primitives culling mode
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GfxCullMode : ur_byte
	{
		None,
		CW,
		CCW
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx rasterizer state description
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GfxRasterizerState
	{
		GfxFillMode FillMode;
		GfxCullMode CullMode;
		ur_int DepthBias;
		ur_float DepthBiasClamp;
		ur_float SlopeScaledDepthBias;
		ur_bool DepthClipEnable;
		ur_bool ScissorEnable;
		ur_bool MultisampleEnable;

		static const GfxRasterizerState Default;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx full render state description
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GfxRenderState
	{
		static const ur_uint MaxRenderTargets = 8;
		static const ur_uint MaxSamplerStates = 16;

		GfxSamplerState SamplerState[MaxSamplerStates];
		GfxRasterizerState RasterizerState;
		GfxDepthStencilState DepthStencilState;
		GfxBlendState BlendState[MaxRenderTargets];

		static const GfxRenderState Default;
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx shader input semantic types
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GfxSemantic : ur_byte
	{
		Position,
		TexCoord,
		Color,
		Tangtent,
		Normal,
		Binormal
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx input layout element description
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GfxInputElement
	{
		GfxSemantic Semantic;
		ur_uint SemanticIdx;
		ur_uint SlotIdx;
		GfxFormat Format;
		GfxFormatView FormatView;
		ur_uint InstanceStepRate;
	};

	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx shader register types
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class UR_DECL GfxShaderRegister : ur_byte
	{
		ReadBuffer = 0,
		ReadWriteBuffer,
		ConstantBuffer,
		Sampler,
		Count
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Gfx adapter description
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct UR_DECL GfxAdapterDesc
	{
		std::wstring Description;
		ur_uint VendorId;
		ur_uint DeviceId;
		ur_uint SubSysId;
		ur_uint Revision;
		ur_size DedicatedVideoMemory;
		ur_size DedicatedSystemMemory;
		ur_size SharedSystemMemory;
	};

} // end namespace UnlimRealms