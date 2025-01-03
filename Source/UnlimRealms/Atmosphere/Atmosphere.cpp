///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Atmosphere/Atmosphere.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"
#include "Sys/Canvas.h"
#include "Resources/Resources.h"
#include "ImguiRender/ImguiRender.h"
#include "GenericRender/GenericRender.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Atmosphere
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	const AtmosphereDesc Atmosphere::DescDefault = {
		1.0f,		// InnerRadius
		1.025f,		// OuterRadius
		0.250f,		// ScaleDepth
		-0.98f,		// G
		0.00005f,	// Km
		0.00005f,	// Kr
		2.718f,		// D
	};

	const AtmosphereDesc Atmosphere::DescInvisible = {
		1.0f,		// InnerRadius
		1.025f,		// OuterRadius
		1.000f,		// ScaleDepth
		-0.98f,		// G
		0.0f,		// Km
		0.0f,		// Kr
		2.718f,		// D
	};

	const Atmosphere::LightShaftsDesc Atmosphere::LightShaftsDesc::Default = {
		0.300f,		// Density
		0.700f,		// DensityMax
		0.070f,		// Weight
		0.970f,		// Decay
		0.250f,		// Exposure
	};

	static const LightDesc LightDescDefault = {
		{ 1.0f, 1.0f, 1.0f },			// Color
		SolarIlluminanceNoon,			// Intensity
		{ 1.0f, 0.0f, 0.0f },			// Direction
		SolarDiskHalfAngleTangent,		// Size
		{ 0.0f, 0.0f, 0.0f },			// Position
		SolarIlluminanceTopAtmosphere,	// IntensityTopAtmosphere
		LightType_Directional,			// Type
	};

	Atmosphere::Atmosphere(Realm &realm) :
		RealmEntity(realm)
	{
		this->lightShafts = LightShaftsDesc::Default;
		#if defined(UR_GRAF)
		this->grafRenderer = realm.GetComponent<GrafRenderer>();
		#endif
	}

	Atmosphere::~Atmosphere()
	{
	#if defined(UR_GRAF)
		if (this->grafObjects != ur_null)
		{
			this->grafRenderer->SafeDelete(this->grafObjects.release());
		}
	#endif
	}

	Result Atmosphere::Init(const AtmosphereDesc &desc)
	{
	#if defined(UR_GRAF)
		return Result(NotImplemented);
	#else
		Result res(Success);

		this->desc = desc;
		res = this->CreateGfxObjects();

		if (Succeeded(res))
		{
			res = this->CreateMesh();
		}

		return res;
	#endif
	}

	Result Atmosphere::Init(const AtmosphereDesc &desc, GrafRenderPass* grafRenderPass)
	{
	#if !defined(UR_GRAF)
		return Result(NotImplemented);
	#else
		Result res(Success);

		this->desc = desc;
		res = this->CreateGrafObjects(grafRenderPass);

		if (Succeeded(res))
		{
			res = this->CreateMesh();
		}

		return res;
	#endif
	}

	#if defined(UR_GRAF)

	Atmosphere::GrafObjects::GrafObjects(GrafSystem& grafSystem) :
		GrafEntity(grafSystem)
	{
	}
	Atmosphere::GrafObjects::~GrafObjects()
	{
	}

	Result Atmosphere::CreateGrafObjects(GrafRenderPass* grafRenderPass)
	{
		Result res = Result(Success);

		if (this->grafObjects != ur_null)
		{
			this->grafRenderer->SafeDelete(this->grafObjects.release());
		}

		if (ur_null == this->grafRenderer)
			return ResultError(InvalidArgs, "Atmosphere::CreateGrafObjects: invalid GrafRenderer");
		if (ur_null == grafRenderPass)
			return ResultError(InvalidArgs, "Atmosphere::CreateGrafObjects: invalid GrafRenderPass");

		GrafSystem* grafSystem = this->grafRenderer->GetGrafSystem();
		GrafDevice* grafDevice = this->grafRenderer->GetGrafDevice();
		ur_uint frameCount = this->grafRenderer->GetRecordedFrameCount();
		std::unique_ptr<GrafObjects> grafObjects(new GrafObjects(*grafSystem));

		// VS
		res = GrafUtils::CreateShaderFromFile(*grafDevice, "Atmosphere_vs", GrafShaderType::Vertex, grafObjects->VS);
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGrafObjects: failed to initialize VS");

		// PS
		res = GrafUtils::CreateShaderFromFile(*grafDevice, "Atmosphere_ps", GrafShaderType::Pixel, grafObjects->PS);
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGrafObjects: failed to initialize PS");

		// VS fullscreen quad
		res = GrafUtils::CreateShaderFromFile(*grafDevice, "FullScreenQuad_vs", GrafShaderType::Vertex, grafObjects->fullScreenQuadVS);
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGrafObjects: failed to create shader for FullScreenQuad_vs");

		// PS fullscreen quad
		res = GrafUtils::CreateShaderFromFile(*grafDevice, "Generic_ps", GrafShaderType::Pixel, grafObjects->fullScreenQuadPS);
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGrafObjects: failed to create shader for Generic_ps");

		// PS LightShafts
		res = GrafUtils::CreateShaderFromFile(*grafDevice, "LightShafts_ps", GrafShaderType::Pixel, grafObjects->lightShaftsPS);
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGrafObjects: failed to initialize LightShafts PS");

		// sampler point
		res = grafSystem->CreateSampler(grafObjects->samplerPoint);
		if (Succeeded(res))
		{
			GrafSamplerDesc samplerDesc = GrafSamplerDesc::Default;
			samplerDesc = {
				GrafFilterType::Nearest, GrafFilterType::Nearest, GrafFilterType::Nearest,
				GrafAddressMode::Clamp, GrafAddressMode::Clamp, GrafAddressMode::Clamp
			};
			res = grafObjects->samplerPoint->Initialize(grafDevice, { samplerDesc });
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGrafObjects: failed to initialize sampler");

		// sampler linear
		res = grafSystem->CreateSampler(grafObjects->samplerLinear);
		if (Succeeded(res))
		{
			GrafSamplerDesc samplerDesc = GrafSamplerDesc::Default;
			samplerDesc = {
				GrafFilterType::Linear, GrafFilterType::Linear, GrafFilterType::Linear,
				GrafAddressMode::Clamp, GrafAddressMode::Clamp, GrafAddressMode::Clamp
			};
			res = grafObjects->samplerLinear->Initialize(grafDevice, { samplerDesc });
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGrafObjects: failed to initialize sampler");

		// shader descriptors layout
		res = grafSystem->CreateDescriptorTableLayout(grafObjects->shaderDescriptorLayout);
		if (Succeeded(res))
		{
			GrafDescriptorRangeDesc grafDescriptorRanges[] = {
				{ GrafDescriptorType::ConstantBuffer, 0, 1 },
			};
			GrafDescriptorTableLayoutDesc grafDescriptorLayoutDesc = {
				GrafShaderStageFlags((ur_uint)GrafShaderStageFlag::Vertex | (ur_uint)GrafShaderStageFlag::Pixel),
				grafDescriptorRanges, ur_array_size(grafDescriptorRanges)
			};
			res = grafObjects->shaderDescriptorLayout->Initialize(grafDevice, { grafDescriptorLayoutDesc });
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGrafObjects: failed to initialize descriptor table layout");

		// fullscreen quad descriptors layout
		res = grafSystem->CreateDescriptorTableLayout(grafObjects->fullscreenQuadDescriptorLayout);
		if (Succeeded(res))
		{
			GrafDescriptorRangeDesc grafDescriptorRanges[] = {
				{ GrafDescriptorType::Sampler, 0, 1 },
				{ GrafDescriptorType::Texture, 0, 1 }
			};
			GrafDescriptorTableLayoutDesc grafDescriptorLayoutDesc = {
				GrafShaderStageFlags(GrafShaderStageFlag::Pixel),
				grafDescriptorRanges, ur_array_size(grafDescriptorRanges)
			};
			res = grafObjects->fullscreenQuadDescriptorLayout->Initialize(grafDevice, { grafDescriptorLayoutDesc });
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGrafObjects: failed to initialize fullscreenQuadDescriptorLayout");

		// light shafts descriptors layout
		res = grafSystem->CreateDescriptorTableLayout(grafObjects->lightShaftsApplyDescriptorLayout);
		if (Succeeded(res))
		{
			GrafDescriptorRangeDesc grafDescriptorRanges[] = {
				{ GrafDescriptorType::ConstantBuffer, 1, 2 },
				{ GrafDescriptorType::Sampler, 0, 1 },
				{ GrafDescriptorType::Texture, 0, 1 }
			};
			GrafDescriptorTableLayoutDesc grafDescriptorLayoutDesc = {
				GrafShaderStageFlags(GrafShaderStageFlag::Pixel),
				grafDescriptorRanges, ur_array_size(grafDescriptorRanges)
			};
			res = grafObjects->lightShaftsApplyDescriptorLayout->Initialize(grafDevice, { grafDescriptorLayoutDesc });
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGrafObjects: failed to initialize lightShaftsApplyDescriptorLayout");

		// per frame descriptor tables
		grafObjects->shaderDescriptorTable.resize(frameCount);
		for (ur_uint iframe = 0; iframe < frameCount; ++iframe)
		{
			res = grafSystem->CreateDescriptorTable(grafObjects->shaderDescriptorTable[iframe]);
			if (Failed(res)) break;
			res = grafObjects->shaderDescriptorTable[iframe]->Initialize(grafDevice, { grafObjects->shaderDescriptorLayout.get() });
			if (Failed(res)) break;
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGrafObjects: failed to initialize descriptor table(s)");

		// per frame LightShafts mask pass descriptor tables
		grafObjects->lightShaftsMaskDescriptorTable.resize(frameCount);
		for (ur_uint iframe = 0; iframe < frameCount; ++iframe)
		{
			res = grafSystem->CreateDescriptorTable(grafObjects->lightShaftsMaskDescriptorTable[iframe]);
			if (Failed(res)) break;
			res = grafObjects->lightShaftsMaskDescriptorTable[iframe]->Initialize(grafDevice, { grafObjects->fullscreenQuadDescriptorLayout.get() });
			if (Failed(res)) break;
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGrafObjects: failed to initialize lightShaftsMaskDescriptorTable(s)");

		// per frame LightShafts apply pass descriptor tables
		grafObjects->lightShaftsApplyDescriptorTable.resize(frameCount);
		for (ur_uint iframe = 0; iframe < frameCount; ++iframe)
		{
			res = grafSystem->CreateDescriptorTable(grafObjects->lightShaftsApplyDescriptorTable[iframe]);
			if (Failed(res)) break;
			res = grafObjects->lightShaftsApplyDescriptorTable[iframe]->Initialize(grafDevice, { grafObjects->lightShaftsApplyDescriptorLayout.get() });
			if (Failed(res)) break;
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGrafObjects: failed to initialize lightShaftsApplyDescriptorTable(s)");

		// LightShafts occlusion mask render pass
		res = grafSystem->CreateRenderPass(grafObjects->lightShaftsMaskRenderPass);
		if (Succeeded(res))
		{
			GrafRenderPassImageDesc renderPassDesc[] = {
				{ // color
					GrafFormat::R16G16B16A16_SFLOAT,
					GrafImageState::Undefined, GrafImageState::ColorWrite,
					GrafRenderPassDataOp::Clear, GrafRenderPassDataOp::Store,
					GrafRenderPassDataOp::DontCare, GrafRenderPassDataOp::DontCare
				},
				{ // depth & stencil
					GrafFormat::D24_UNORM_S8_UINT,
					GrafImageState::DepthStencilWrite, GrafImageState::DepthStencilWrite,
					GrafRenderPassDataOp::Load, GrafRenderPassDataOp::Store,
					GrafRenderPassDataOp::Load, GrafRenderPassDataOp::Store
				}
			};
			res = grafObjects->lightShaftsMaskRenderPass->Initialize(grafDevice, { renderPassDesc, 2u });
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGrafObjects: failed to initialize lightShaftsMaskRenderPass");

		// LightShafts apply render pass
		res = grafSystem->CreateRenderPass(grafObjects->lightShaftsApplyRenderPass);
		if (Succeeded(res))
		{
			GrafRenderPassImageDesc renderPassDesc[] = {
				{ // color
					GrafFormat::R16G16B16A16_SFLOAT,
					GrafImageState::ColorWrite, GrafImageState::ColorWrite,
					GrafRenderPassDataOp::Load, GrafRenderPassDataOp::Store,
					GrafRenderPassDataOp::DontCare, GrafRenderPassDataOp::DontCare
				},
				{ // depth & stencil
					GrafFormat::D24_UNORM_S8_UINT,
					GrafImageState::DepthStencilWrite, GrafImageState::DepthStencilWrite,
					GrafRenderPassDataOp::Load, GrafRenderPassDataOp::Store,
					GrafRenderPassDataOp::Load, GrafRenderPassDataOp::Store
				}
			};
			res = grafObjects->lightShaftsApplyRenderPass->Initialize(grafDevice, { renderPassDesc, 2u });
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGrafObjects: failed to initialize lightShaftsApplyRenderPass");

		// graphics pipeline configuration
		res = grafSystem->CreatePipeline(grafObjects->pipelineSolid);
		if (Succeeded(res))
		{
			GrafShader* shaderStages[] = {
				grafObjects->VS.get(),
				grafObjects->PS.get()
			};
			GrafDescriptorTableLayout* descriptorLayouts[] = {
				grafObjects->shaderDescriptorLayout.get(),
			};
			GrafVertexElementDesc vertexElements[] = {
				{ GrafFormat::R32G32B32_SFLOAT, 0, "POSITION" }
			};
			GrafVertexInputDesc vertexInputs[] = { {
				GrafVertexInputType::PerVertex, 0, sizeof(Vertex),
				vertexElements, ur_array_size(vertexElements)
			} };
			GrafColorBlendOpDesc colorBlendOpDesc = {
				GrafColorChannelFlags(GrafColorChannelFlag::All), true,
				GrafBlendOp::Add, GrafBlendFactor::SrcAlpha, GrafBlendFactor::One,
				GrafBlendOp::Add, GrafBlendFactor::SrcAlpha, GrafBlendFactor::One,
			};
			GrafPipeline::InitParams pipelineParams = GrafPipeline::InitParams::Default;
			pipelineParams.RenderPass = grafRenderPass;
			pipelineParams.ShaderStages = shaderStages;
			pipelineParams.ShaderStageCount = ur_array_size(shaderStages);
			pipelineParams.DescriptorTableLayouts = descriptorLayouts;
			pipelineParams.DescriptorTableLayoutCount = ur_array_size(descriptorLayouts);
			pipelineParams.VertexInputDesc = vertexInputs;
			pipelineParams.VertexInputCount = ur_array_size(vertexInputs);
			pipelineParams.PrimitiveTopology = GrafPrimitiveTopology::TriangleList;
			pipelineParams.FrontFaceOrder = GrafFrontFaceOrder::Clockwise;
			pipelineParams.CullMode = GrafCullMode::Front;
			pipelineParams.DepthTestEnable = true;
			pipelineParams.DepthWriteEnable = false;
			pipelineParams.DepthCompareOp = GrafCompareOp::LessOrEqual;
			pipelineParams.StencilTestEnable = true;
			pipelineParams.StencilBack.WriteMask = ur_uint32(~0);
			pipelineParams.StencilBack.CompareOp = GrafCompareOp::Always;
			pipelineParams.StencilBack.PassOp = GrafStencilOp::Replace;
			pipelineParams.StencilBack.Reference = 0x1;
			pipelineParams.ColorBlendOpDescCount = 1;
			pipelineParams.ColorBlendOpDesc = &colorBlendOpDesc;
			res = grafObjects->pipelineSolid->Initialize(grafDevice, pipelineParams);
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGrafObjects: failed to initialize pipeline object");

		// LightShafts mask pass pipeline configuration
		res = grafSystem->CreatePipeline(grafObjects->pipelineLightShaftsMask);
		if (Succeeded(res))
		{
			GrafShader* shaderStages[] = {
				grafObjects->fullScreenQuadVS.get(),
				grafObjects->fullScreenQuadPS.get()
			};
			GrafDescriptorTableLayout* descriptorLayouts[] = {
				grafObjects->fullscreenQuadDescriptorLayout.get(),
			};
			GrafPipeline::InitParams pipelineParams = GrafPipeline::InitParams::Default;
			pipelineParams.RenderPass = grafObjects->lightShaftsMaskRenderPass.get();
			pipelineParams.ShaderStages = shaderStages;
			pipelineParams.ShaderStageCount = ur_array_size(shaderStages);
			pipelineParams.DescriptorTableLayouts = descriptorLayouts;
			pipelineParams.DescriptorTableLayoutCount = ur_array_size(descriptorLayouts);
			pipelineParams.PrimitiveTopology = GrafPrimitiveTopology::TriangleStrip;
			pipelineParams.FrontFaceOrder = GrafFrontFaceOrder::Clockwise;
			pipelineParams.CullMode = GrafCullMode::Back;
			pipelineParams.DepthTestEnable = false;
			pipelineParams.DepthWriteEnable = false;
			pipelineParams.StencilTestEnable = true;
			pipelineParams.StencilFront.WriteMask = 0x0;
			pipelineParams.StencilFront.CompareOp = GrafCompareOp::Equal;
			pipelineParams.StencilFront.CompareMask = 0x1;
			pipelineParams.StencilFront.Reference = 0x1;
			res = grafObjects->pipelineLightShaftsMask->Initialize(grafDevice, pipelineParams);
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGrafObjects: failed to initialize pipelineLightShaftsMask object");

		// LightShafts apply pass pipeline configuration
		res = grafSystem->CreatePipeline(grafObjects->pipelineLightShaftsApply);
		if (Succeeded(res))
		{
			GrafShader* shaderStages[] = {
				grafObjects->fullScreenQuadVS.get(),
				grafObjects->lightShaftsPS.get()
			};
			GrafDescriptorTableLayout* descriptorLayouts[] = {
				grafObjects->lightShaftsApplyDescriptorLayout.get(),
			};
			GrafColorBlendOpDesc colorBlendOpDesc = {
				GrafColorChannelFlags(GrafColorChannelFlag::All), true,
				GrafBlendOp::Add, GrafBlendFactor::SrcAlpha, GrafBlendFactor::InvSrcAlpha,
				GrafBlendOp::Add, GrafBlendFactor::SrcAlpha, GrafBlendFactor::InvSrcAlpha,
			};
			GrafPipeline::InitParams pipelineParams = GrafPipeline::InitParams::Default;
			pipelineParams.RenderPass = grafObjects->lightShaftsApplyRenderPass.get();
			pipelineParams.ShaderStages = shaderStages;
			pipelineParams.ShaderStageCount = ur_array_size(shaderStages);
			pipelineParams.DescriptorTableLayouts = descriptorLayouts;
			pipelineParams.DescriptorTableLayoutCount = ur_array_size(descriptorLayouts);
			pipelineParams.PrimitiveTopology = GrafPrimitiveTopology::TriangleStrip;
			pipelineParams.FrontFaceOrder = GrafFrontFaceOrder::Clockwise;
			pipelineParams.CullMode = GrafCullMode::Back;
			pipelineParams.ColorBlendOpDescCount = 1;
			pipelineParams.ColorBlendOpDesc = &colorBlendOpDesc;
			res = grafObjects->pipelineLightShaftsApply->Initialize(grafDevice, pipelineParams);
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGrafObjects: failed to initialize pipelineLightShaftsApply object");

		this->grafObjects = std::move(grafObjects);

		return res;
	}

	#else

	Result Atmosphere::CreateGfxObjects()
	{
		Result res = Result(Success);

		this->gfxObjects = GfxObjects(); // reset gfx resources

		// VS
		res = CreateVertexShaderFromFile(this->GetRealm(), this->gfxObjects.VS, "Atmosphere_vs.cso");
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGfxObjects: failed to initialize VS");

		// PS
		res = CreatePixelShaderFromFile(this->GetRealm(), this->gfxObjects.PS, "Atmosphere_ps.cso");
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGfxObjects: failed to initialize PS");

		// Input Layout
		res = this->GetRealm().GetGfxSystem()->CreateInputLayout(this->gfxObjects.inputLayout);
		if (Succeeded(res))
		{
			GfxInputElement elements[] = {
				{ GfxSemantic::Position, 0, 0, GfxFormat::R32G32B32, GfxFormatView::Float, 0 }
			};
			res = this->gfxObjects.inputLayout->Initialize(*this->gfxObjects.VS.get(), elements, ur_array_size(elements));
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGfxObjects: failed to initialize input layout");

		// Pipeline State
		res = this->GetRealm().GetGfxSystem()->CreatePipelineState(this->gfxObjects.pipelineState);
		if (Succeeded(res))
		{
			this->gfxObjects.pipelineState->InputLayout = this->gfxObjects.inputLayout.get();
			this->gfxObjects.pipelineState->VertexShader = this->gfxObjects.VS.get();
			this->gfxObjects.pipelineState->PixelShader = this->gfxObjects.PS.get();
			GfxRenderState gfxRS = GfxRenderState::Default;
			gfxRS.RasterizerState.CullMode = GfxCullMode::CW;
			gfxRS.RasterizerState.FillMode = GfxFillMode::Solid;
			gfxRS.BlendState[0].BlendEnable = true;
			gfxRS.BlendState[0].SrcBlend = GfxBlendFactor::SrcAlpha;
			gfxRS.BlendState[0].SrcBlendAlpha = GfxBlendFactor::SrcAlpha;
			gfxRS.BlendState[0].DstBlend = GfxBlendFactor::One;
			gfxRS.BlendState[0].DstBlendAlpha = GfxBlendFactor::One;
			gfxRS.DepthStencilState.DepthWriteEnable = false;
			gfxRS.DepthStencilState.StencilEnable = true;
			gfxRS.DepthStencilState.BackFace.StencilPassOp = GfxStencilOp::Replace;
			this->gfxObjects.pipelineState->StencilRef = 0x1;
			res = this->gfxObjects.pipelineState->SetRenderState(gfxRS);
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGfxObjects: failed to initialize pipeline state");

		// Constant Buffer
		res = this->GetRealm().GetGfxSystem()->CreateBuffer(this->gfxObjects.CB);
		if (Succeeded(res))
		{
			res = this->gfxObjects.CB->Initialize(sizeof(CommonCB), 0, GfxUsage::Dynamic,
				(ur_uint)GfxBindFlag::ConstantBuffer, (ur_uint)GfxAccessFlag::Write);
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGfxObjects: failed to initialize constant buffer");

		// Light Shafts PS
		res = CreatePixelShaderFromFile(this->GetRealm(), this->gfxObjects.lightShaftsPS, "LightShafts_ps.cso");
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGfxObjects: failed to initialize Light Shafts PS");

		// Light Shafts RT
		res = this->GetRealm().GetGfxSystem()->CreateRenderTarget(this->gfxObjects.lightShaftsRT);
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGfxObjects: failed to create Light Shafts render target");

		// Light Shafts CB
		res = this->GetRealm().GetGfxSystem()->CreateBuffer(this->gfxObjects.lightShaftsCB);
		if (Succeeded(res))
		{
			res = this->gfxObjects.lightShaftsCB->Initialize(sizeof(LightShaftsCB), 0, GfxUsage::Dynamic,
				(ur_uint)GfxBindFlag::ConstantBuffer, (ur_uint)GfxAccessFlag::Write);
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGfxObjects: failed to initialize Light Shafts constant buffer");

		// Custom screen quad render states
		GenericRender *genericRender = this->GetRealm().GetComponent<GenericRender>();
		res = (genericRender != ur_null);
		if (Succeeded(res))
		{
			// Occlusion mask
			GfxRenderState occlusionMaskRS = genericRender->GetDefaultQuadRenderState();
			occlusionMaskRS.SamplerState[0].MinFilter = GfxFilter::Point;
			occlusionMaskRS.SamplerState[0].MagFilter = GfxFilter::Point;
			occlusionMaskRS.DepthStencilState.StencilEnable = true;
			occlusionMaskRS.DepthStencilState.FrontFace.StencilFunc = GfxCmpFunc::Equal;
			occlusionMaskRS.DepthStencilState.StencilWriteMask = 0x0;
			genericRender->CreateScreenQuadState(this->gfxObjects.screenQuadStateOcclusionMask,
				ur_null, &occlusionMaskRS, 0x1);

			// Blend light shafts 
			GfxRenderState lightShaftsBlendRS = genericRender->GetDefaultQuadRenderState();
			lightShaftsBlendRS.BlendState[0].BlendEnable = true;
			lightShaftsBlendRS.BlendState[0].SrcBlend = GfxBlendFactor::SrcAlpha;
			lightShaftsBlendRS.BlendState[0].SrcBlendAlpha = GfxBlendFactor::SrcAlpha;
			lightShaftsBlendRS.BlendState[0].DstBlend = GfxBlendFactor::InvSrcAlpha;
			lightShaftsBlendRS.BlendState[0].DstBlendAlpha = GfxBlendFactor::InvSrcAlpha;
			genericRender->CreateScreenQuadState(this->gfxObjects.screenQuadStateBlendLightShafts,
				this->gfxObjects.lightShaftsPS.get(), &lightShaftsBlendRS);
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGfxObjects: failed to initialize Light Shafts render states");

		return res;
	}
	#endif

	Result Atmosphere::CreateMesh()
	{
		Result res = Result(Success);

		// compute mesh data
		const ur_uint gridDetail = 127;
		std::vector<Atmosphere::Vertex> vertices(gridDetail * gridDetail);
		Atmosphere::Vertex *pv = vertices.data();
		ur_float ay, ax, y, r;
		for (ur_uint iy = 0; iy < gridDetail; ++iy)
		{
			ay = ur_float(iy) / (gridDetail - 1) * MathConst<float>::Pi - MathConst<float>::PiHalf;
			y = this->desc.OuterRadius * sin(ay);
			for (ur_uint ix = 0; ix < gridDetail; ++ix, ++pv)
			{
				ax = ur_float(ix) / gridDetail * MathConst<float>::Pi * 2.0f;
				r = this->desc.OuterRadius * cos(ay);
				pv->pos.y = y;
				pv->pos.x = cos(ax) * r;
				pv->pos.z = sin(ax) * r;
			}
		}
		std::vector<Atmosphere::Index> indices(gridDetail * (gridDetail - 1) * 6);
		Atmosphere::Index *pi = indices.data();
		ur_uint ix_next;
		for (ur_uint iy = 0; iy < gridDetail - 1; ++iy)
		{
			for (ur_uint ix = 0; ix < gridDetail; ++ix)
			{
				ix_next = (ix + 1 < gridDetail ? ix + 1 : 0);
				*pi++ = ix + iy * gridDetail;
				*pi++ = ix + (iy + 1) * gridDetail;
				*pi++ = ix_next + (iy + 1) * gridDetail;
				*pi++ = ix_next + (iy + 1) * gridDetail;
				*pi++ = ix_next + iy * gridDetail;
				*pi++ = ix + iy * gridDetail;
			}
		}

		#if defined(UR_GRAF)

		GrafSystem* grafSystem = this->grafRenderer->GetGrafSystem();
		GrafDevice* grafDevice = this->grafRenderer->GetGrafDevice();

		// create vertex buffer
		res = grafSystem->CreateBuffer(this->grafObjects->VB);
		if (Succeeded(res))
		{
			GrafBufferDesc bufferDesc = {};
			bufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::VertexBuffer | (ur_uint)GrafBufferUsageFlag::TransferDst;
			bufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::GpuLocal;
			bufferDesc.SizeInBytes = (ur_size)vertices.size() * sizeof(Atmosphere::Vertex);
			bufferDesc.ElementSize = sizeof(Atmosphere::Vertex);
			res = this->grafObjects->VB->Initialize(grafDevice, { bufferDesc });
			if (Succeeded(res))
			{
				grafRenderer->Upload((ur_byte*)vertices.data(), this->grafObjects->VB.get(), bufferDesc.SizeInBytes);
			}
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateMesh: failed to initialize VB");

		// create index buffer
		res = grafSystem->CreateBuffer(this->grafObjects->IB);
		if (Succeeded(res))
		{
			GrafBufferDesc bufferDesc = {};
			bufferDesc.Usage = (ur_uint)GrafBufferUsageFlag::IndexBuffer | (ur_uint)GrafBufferUsageFlag::TransferDst;
			bufferDesc.MemoryType = (ur_uint)GrafDeviceMemoryFlag::GpuLocal;
			bufferDesc.SizeInBytes = (ur_size)indices.size() * sizeof(Atmosphere::Index);
			bufferDesc.ElementSize = sizeof(Atmosphere::Index);
			res = this->grafObjects->IB->Initialize(grafDevice, { bufferDesc });
			if (Succeeded(res))
			{
				grafRenderer->Upload((ur_byte*)indices.data(), this->grafObjects->IB.get(), bufferDesc.SizeInBytes);
			}
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateMesh: failed to initialize IB");

		#else

		// create vertex buffer
		res = this->GetRealm().GetGfxSystem()->CreateBuffer(this->gfxObjects.VB);
		if (Succeeded(res))
		{
			GfxResourceData gfxRes = { vertices.data(), (ur_uint)vertices.size() * sizeof(Atmosphere::Vertex), 0 };
			res = this->gfxObjects.VB->Initialize(gfxRes.RowPitch, sizeof(Atmosphere::Vertex), GfxUsage::Immutable, (ur_uint)GfxBindFlag::VertexBuffer, 0, &gfxRes);
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateMesh: failed to initialize VB");

		// create index buffer
		res = this->GetRealm().GetGfxSystem()->CreateBuffer(this->gfxObjects.IB);
		if (Succeeded(res))
		{
			GfxResourceData gfxRes = { indices.data(), (ur_uint)indices.size() * sizeof(Atmosphere::Index), 0 };
			res = this->gfxObjects.IB->Initialize(gfxRes.RowPitch, sizeof(Atmosphere::Index), GfxUsage::Immutable, (ur_uint)GfxBindFlag::IndexBuffer, 0, &gfxRes);
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateMesh: failed to initialize IB");

		#endif

		return Result(Success);
	}

	Result Atmosphere::Render(GfxContext &gfxContext, const ur_float4x4 &viewProj, const ur_float3 &cameraPos, const LightDesc* light)
	{
	#if defined(UR_GRAF)
		return Result(NotImplemented);
	#else
		if (ur_null == this->gfxObjects.VB ||
			ur_null == this->gfxObjects.IB)
			return NotInitialized;

		// constants
		CommonCB cb;
		cb.ViewProj = viewProj;
		cb.CameraPos = cameraPos;
		cb.Params = this->desc;
		cb.Light = (light != ur_null ? *light : LightDescDefault);
		GfxResourceData cbResData = { &cb, sizeof(CommonCB), 0 };
		gfxContext.UpdateBuffer(this->gfxObjects.CB.get(), GfxGPUAccess::WriteDiscard, &cbResData, 0, cbResData.RowPitch);
		gfxContext.SetConstantBuffer(this->gfxObjects.CB.get(), 0);

		// pipeline state
		gfxContext.SetPipelineState(this->gfxObjects.pipelineState.get());

		// draw
		const ur_uint indexCount = this->gfxObjects.IB->GetDesc().Size / sizeof(Atmosphere::Index);
		gfxContext.SetVertexBuffer(this->gfxObjects.VB.get(), 0);
		gfxContext.SetIndexBuffer(this->gfxObjects.IB.get());
		gfxContext.DrawIndexed(indexCount, 0, 0, 0, 0);

		return Success;
	#endif
	}

	Result Atmosphere::RenderPostEffects(GfxContext &gfxContext, GfxRenderTarget &renderTarget,
		const ur_float4x4 &viewProj, const ur_float3 &cameraPos, const LightDesc* light)
	{
	#if defined(UR_GRAF)
		return Result(NotImplemented);
	#else
		GenericRender *genericRender = this->GetRealm().GetComponent<GenericRender>();
		if (ur_null == this->gfxObjects.lightShaftsRT || ur_null == genericRender)
			return NotInitialized;

		if (ur_null == renderTarget.GetTargetBuffer())
			return InvalidArgs;

		Result res = Success;

		// (re)init post process RT
		const GfxTextureDesc &targetDesc = renderTarget.GetTargetBuffer()->GetDesc();
		if (this->gfxObjects.lightShaftsRT == ur_null || this->gfxObjects.lightShaftsRT->GetTargetBuffer() == ur_null ||
			this->gfxObjects.lightShaftsRT->GetTargetBuffer()->GetDesc().Width != targetDesc.Width ||
			this->gfxObjects.lightShaftsRT->GetTargetBuffer()->GetDesc().Height != targetDesc.Height)
		{
			GfxTextureDesc descRT;
			descRT.Width = targetDesc.Width;
			descRT.Height = targetDesc.Height;
			descRT.Levels = 1;
			descRT.Format = targetDesc.Format;
			descRT.FormatView = targetDesc.FormatView;
			descRT.Usage = GfxUsage::Default;
			descRT.BindFlags = ur_uint(GfxBindFlag::RenderTarget) | ur_uint(GfxBindFlag::ShaderResource);
			descRT.AccessFlags = ur_uint(0);
			res &= this->gfxObjects.lightShaftsRT->Initialize(descRT, false, GfxFormat::Unknown);
			if (Failed(res))
				return ResultError(Failure, "Atmosphere::RenderPostEffects: failed to (re)init Light Shafts render target");
		}

		// constants
		GfxResourceData cbResData = { &this->lightShafts, sizeof(LightShaftsCB), 0 };
		res &= gfxContext.UpdateBuffer(this->gfxObjects.lightShaftsCB.get(), GfxGPUAccess::WriteDiscard, &cbResData, 0, cbResData.RowPitch);
		res &= gfxContext.SetConstantBuffer(this->gfxObjects.CB.get(), 1);
		res &= gfxContext.SetConstantBuffer(this->gfxObjects.lightShaftsCB.get(), 2);

		// draw atmosphere into separate RT and mask out occlusion fragments using atmosphere's stencil ref
		res &= gfxContext.SetRenderTarget(this->gfxObjects.lightShaftsRT.get(), &renderTarget);
		res &= gfxContext.ClearTarget(this->gfxObjects.lightShaftsRT.get(), true, { 0.0f, 0.0f, 0.0f, 0.0f }, false, 0, false, 0);
		res &= genericRender->RenderScreenQuad(gfxContext, renderTarget.GetTargetBuffer(), ur_null,
			this->gfxObjects.screenQuadStateOcclusionMask.get());
		gfxContext.SetRenderTarget(ur_null);

		// draw lights shafts into given RT
		res &= gfxContext.SetRenderTarget(&renderTarget);
		res &= genericRender->RenderScreenQuad(gfxContext, this->gfxObjects.lightShaftsRT->GetTargetBuffer(), ur_null,
			this->gfxObjects.screenQuadStateBlendLightShafts.get());

		return res;
	#endif
	}

	Result Atmosphere::Render(GrafCommandList &grafCmdList, const ur_float4x4 &viewProj, const ur_float3 &cameraPos, const LightDesc* light)
	{
	#if !defined(UR_GRAF)
		return Result(NotImplemented);
	#else
		if (ur_null == this->grafObjects)
			return NotInitialized;

		Result res = Success;

		// fill CB
		CommonCB cbData;
		cbData.ViewProj = viewProj;
		cbData.CameraPos = cameraPos;
		cbData.Params = this->desc;
		cbData.Light = (light != ur_null ? *light : LightDescDefault);
		GrafBuffer* dynamicCB = this->grafRenderer->GetDynamicConstantBuffer();
		Allocation dynamicCBAlloc = grafRenderer->GetDynamicConstantBufferAllocation(sizeof(CommonCB));
		dynamicCB->Write((ur_byte*)&cbData, sizeof(cbData), 0, dynamicCBAlloc.Offset);

		// update shader descriptors
		ur_uint frameIdx = this->grafRenderer->GetRecordedFrameIdx();
		GrafDescriptorTable *shaderDescriptorTable = this->grafObjects->shaderDescriptorTable[frameIdx].get();
		shaderDescriptorTable->SetConstantBuffer(0, dynamicCB, dynamicCBAlloc.Offset, dynamicCBAlloc.Size);

		// bind pipeline
		const ur_uint indexCount = ur_uint(this->grafObjects->IB->GetDesc().SizeInBytes / sizeof(Atmosphere::Index));
		grafCmdList.BindVertexBuffer(this->grafObjects->VB.get(), 0);
		grafCmdList.BindIndexBuffer(this->grafObjects->IB.get(), (sizeof(Atmosphere::Index) == 2 ? GrafIndexType::UINT16 : GrafIndexType::UINT32));
		grafCmdList.BindPipeline(this->grafObjects->pipelineSolid.get());
		grafCmdList.BindDescriptorTable(shaderDescriptorTable, this->grafObjects->pipelineSolid.get());

		// draw
		grafCmdList.DrawIndexed(indexCount, 1, 0, 0, 0);

		return res;
	#endif
	}

	Result Atmosphere::RenderPostEffects(GrafCommandList &grafCmdList, GrafRenderTarget &renderTarget,
		const ur_float4x4 &viewProj, const ur_float3 &cameraPos, const LightDesc* light)
	{
	#if !defined(UR_GRAF)
		return Result(NotImplemented);
	#else
		if (ur_null == this->grafObjects)
			return NotInitialized;
		if (renderTarget.GetImageCount() < 2)
			return InvalidArgs; // color & stencil images are expected

		// (re)init post process image(s)

		Result res(Success);
		if (this->grafObjects->lightShaftsRTImage == ur_null ||
			this->grafObjects->lightShaftsRTImage->GetDesc().Size != renderTarget.GetImage(0)->GetDesc().Size)
		{
			if (this->grafObjects->lightShaftsRTImage) this->grafRenderer->SafeDelete(this->grafObjects->lightShaftsRTImage.release(), &grafCmdList);
			if (this->grafObjects->lightShaftsRT) this->grafRenderer->SafeDelete(this->grafObjects->lightShaftsRT.release(), &grafCmdList);
			
			res = this->grafRenderer->GetGrafSystem()->CreateImage(this->grafObjects->lightShaftsRTImage);
			if (Succeeded(res))
			{
				GrafImageDesc lightShaftsImageDesc = {
					GrafImageType::Tex2D,
					GrafFormat::R16G16B16A16_SFLOAT,
					renderTarget.GetImage(0)->GetDesc().Size, 1,
					ur_uint(GrafImageUsageFlag::ColorRenderTarget) | ur_uint(GrafImageUsageFlag::ShaderRead),
					ur_uint(GrafDeviceMemoryFlag::GpuLocal)
				};
				res = this->grafObjects->lightShaftsRTImage->Initialize(this->grafRenderer->GetGrafDevice(), { lightShaftsImageDesc });
			}
			if (Failed(res))
			{
				this->grafObjects->lightShaftsRTImage.reset();
				return ResultError(Failure, "Atmosphere::RenderPostEffects: failed to (re)init lightShaftsRTImage");
			}

			res = this->grafRenderer->GetGrafSystem()->CreateRenderTarget(this->grafObjects->lightShaftsRT);
			if (Succeeded(res))
			{
				GrafImage* lightShaftsRTImages[] = {
					this->grafObjects->lightShaftsRTImage.get(), // destination masked light
					renderTarget.GetImage(1) // depth & stencil image from the main pass used to render atmosphere
				};
				res = this->grafObjects->lightShaftsRT->Initialize(this->grafRenderer->GetGrafDevice(), { grafObjects->lightShaftsMaskRenderPass.get(), lightShaftsRTImages, 2u });
			}
			if (Failed(res))
			{
				this->grafObjects->lightShaftsRTImage.reset();
				this->grafObjects->lightShaftsRT.reset();
				return ResultError(Failure, "Atmosphere::RenderPostEffects: failed to (re)init lightShaftsRT");
			}
		}

		// draw atmosphere into separate RT and mask out occlusion fragments using atmosphere's stencil ref

		GrafViewportDesc rtViewport = {};
		rtViewport.Width = (ur_float)this->grafObjects->lightShaftsRTImage->GetDesc().Size.x;
		rtViewport.Height = (ur_float)this->grafObjects->lightShaftsRTImage->GetDesc().Size.y;
		rtViewport.Near = 0.0f;
		rtViewport.Far = 1.0f;

		static GrafClearValue rtClearValues[] = {
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 1.0f, 0.0f, 0.0f, 0.0f }
		};

		GrafDescriptorTable* descTable = this->grafObjects->lightShaftsMaskDescriptorTable[this->grafRenderer->GetRecordedFrameIdx()].get();
		descTable->SetSampler(0, this->grafObjects->samplerPoint.get());
		descTable->SetImage(0, renderTarget.GetImage(0));

		grafCmdList.SetViewport(rtViewport, true);
		grafCmdList.ImageMemoryBarrier(renderTarget.GetImage(0), GrafImageState::Current, GrafImageState::ShaderRead);
		grafCmdList.ImageMemoryBarrier(this->grafObjects->lightShaftsRTImage.get(), GrafImageState::Current, GrafImageState::ColorWrite);
		grafCmdList.BeginRenderPass(this->grafObjects->lightShaftsMaskRenderPass.get(), this->grafObjects->lightShaftsRT.get(), rtClearValues);
		grafCmdList.BindPipeline(this->grafObjects->pipelineLightShaftsMask.get());
		grafCmdList.BindDescriptorTable(descTable, this->grafObjects->pipelineLightShaftsMask.get());
		grafCmdList.Draw(4, 1, 0, 0);
		grafCmdList.EndRenderPass();

		// draw lights shafts into given RT

		GrafBuffer* dynamicCB = this->grafRenderer->GetDynamicConstantBuffer();
		CommonCB cbData;
		cbData.ViewProj = viewProj;
		cbData.CameraPos = cameraPos;
		cbData.Params = this->desc;
		cbData.Light = (light != ur_null ? *light : LightDescDefault);
		Allocation commonCBAlloc = grafRenderer->GetDynamicConstantBufferAllocation(sizeof(CommonCB));
		dynamicCB->Write((ur_byte*)&cbData, sizeof(cbData), 0, commonCBAlloc.Offset);
		Allocation lightShaftsCBAlloc = grafRenderer->GetDynamicConstantBufferAllocation(sizeof(LightShaftsCB));
		dynamicCB->Write((ur_byte*)&this->lightShafts, sizeof(this->lightShafts), 0, lightShaftsCBAlloc.Offset);

		descTable = this->grafObjects->lightShaftsApplyDescriptorTable[this->grafRenderer->GetRecordedFrameIdx()].get();
		descTable->SetConstantBuffer(1, dynamicCB, commonCBAlloc.Offset, commonCBAlloc.Size);
		descTable->SetConstantBuffer(2, dynamicCB, lightShaftsCBAlloc.Offset, lightShaftsCBAlloc.Size);
		descTable->SetSampler(0, this->grafObjects->samplerLinear.get());
		descTable->SetImage(0, this->grafObjects->lightShaftsRTImage.get());

		grafCmdList.ImageMemoryBarrier(renderTarget.GetImage(0), GrafImageState::Current, GrafImageState::ColorWrite);
		grafCmdList.ImageMemoryBarrier(this->grafObjects->lightShaftsRTImage.get(), GrafImageState::Current, GrafImageState::ShaderRead);
		grafCmdList.BeginRenderPass(this->grafObjects->lightShaftsApplyRenderPass.get(), &renderTarget);
		grafCmdList.BindPipeline(this->grafObjects->pipelineLightShaftsApply.get());
		grafCmdList.BindDescriptorTable(descTable, this->grafObjects->pipelineLightShaftsApply.get());
		grafCmdList.Draw(4, 1, 0, 0);
		grafCmdList.EndRenderPass();

		return Result(Success);
	#endif
	}

	void Atmosphere::DisplayImgui()
	{
		if (ImGui::CollapsingHeader("Atmosphere"))
		{
			ImGui::InputFloat("InnerRadius", &this->desc.InnerRadius);
			ImGui::InputFloat("OuterRadius", &this->desc.OuterRadius);
			ImGui::DragFloat("ScaleDepth", &this->desc.ScaleDepth, 0.01f, 0.0f, 1.0f);
			ImGui::InputFloat("Kr", &this->desc.Kr);
			ImGui::InputFloat("Km", &this->desc.Km);
			ImGui::DragFloat("G", &this->desc.G, 0.01f, -1.0f, 1.0f);
			ImGui::InputFloat("D", &this->desc.D);

			if (ImGui::TreeNode("LightShafts"))
			{
				ImGui::DragFloat("Density", &this->lightShafts.Density, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("DensityMax", &this->lightShafts.DensityMax, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Weight", &this->lightShafts.Weight, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Decay", &this->lightShafts.Decay, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Exposure", &this->lightShafts.Exposure, 0.01f, 0.0f, 1.0f);
				ImGui::TreePop();
			}
		}
	}

} // end namespace UnlimRealms