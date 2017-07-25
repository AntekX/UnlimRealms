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

	const Atmosphere::Desc Atmosphere::Desc::Default = {
		1.0f,		// InnerRadius
		1.025f,		// OuterRadius
		0.250f,		// ScaleDepth
		-0.98f,		// G
		0.00005f,	// Km
		0.00005f,	// Kr
		2.718f,		// D
	};

	const Atmosphere::Desc Atmosphere::Desc::Invisible = {
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

	Atmosphere::Atmosphere(Realm &realm) :
		RealmEntity(realm)
	{
		this->lightShafts = LightShaftsDesc::Default;
	}

	Atmosphere::~Atmosphere()
	{
	}

	Result Atmosphere::Init(const Desc &desc)
	{
		Result res(Success);

		this->desc = desc;

		res = this->CreateGfxObjects();

		if (Succeeded(res))
		{
			res = this->CreateMesh();
		}

		return res;
	}

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
			res = this->gfxObjects.CB->Initialize(sizeof(CommonCB), GfxUsage::Dynamic,
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
			res = this->gfxObjects.lightShaftsCB->Initialize(sizeof(LightShaftsCB), GfxUsage::Dynamic,
				(ur_uint)GfxBindFlag::ConstantBuffer, (ur_uint)GfxAccessFlag::Write);
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateGfxObjects: failed to initialize Light Shafts constant buffer");

		// Custom screen quad render states
		GenericRender *genericRender = this->GetRealm().GetComponent<GenericRender>();
		res = (genericRender != ur_null);
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

		// create vertex buffer
		res = this->GetRealm().GetGfxSystem()->CreateBuffer(this->gfxObjects.VB);
		if (Succeeded(res))
		{
			GfxResourceData gfxRes = { vertices.data(), (ur_uint)vertices.size() * sizeof(Atmosphere::Vertex), 0 };
			res = this->gfxObjects.VB->Initialize(gfxRes.RowPitch, GfxUsage::Immutable, (ur_uint)GfxBindFlag::VertexBuffer, 0, &gfxRes);
		}
		if (Failed(res))
			return ResultError(Failure, "Atmosphere::CreateMesh: failed to initialize VB");

		// create index buffer
		res = this->GetRealm().GetGfxSystem()->CreateBuffer(this->gfxObjects.IB);
		if (Succeeded(res))
		{
			GfxResourceData gfxRes = { indices.data(), (ur_uint)indices.size() * sizeof(Atmosphere::Index), 0 };
			res = this->gfxObjects.IB->Initialize(gfxRes.RowPitch, GfxUsage::Immutable, (ur_uint)GfxBindFlag::IndexBuffer, 0, &gfxRes);
		}
		if (Failed(res))
			return Result(Failure);

		return Result(Success);
	}

	Result Atmosphere::Render(GfxContext &gfxContext, const ur_float4x4 &viewProj, const ur_float3 &cameraPos)
	{
		if (ur_null == this->gfxObjects.VB ||
			ur_null == this->gfxObjects.IB)
			return NotInitialized;

		// constants
		CommonCB cb;
		cb.ViewProj = viewProj;
		cb.CameraPos = cameraPos;
		cb.Params = this->desc;
		GfxResourceData cbResData = { &cb, sizeof(CommonCB), 0 };
		gfxContext.UpdateBuffer(this->gfxObjects.CB.get(), GfxGPUAccess::WriteDiscard, false, &cbResData, 0, cbResData.RowPitch);
		gfxContext.SetConstantBuffer(this->gfxObjects.CB.get(), 0);

		// pipeline state
		gfxContext.SetPipelineState(this->gfxObjects.pipelineState.get());

		// draw
		const ur_uint indexCount = this->gfxObjects.IB->GetDesc().Size / sizeof(Atmosphere::Index);
		gfxContext.SetVertexBuffer(this->gfxObjects.VB.get(), 0, sizeof(Atmosphere::Vertex), 0);
		gfxContext.SetIndexBuffer(this->gfxObjects.IB.get(), sizeof(Atmosphere::Index) * 8, 0);
		gfxContext.DrawIndexed(indexCount, 0, 0, 0, 0);

		return Success;
	}

	Result Atmosphere::RenderPostEffects(GfxContext &gfxContext, GfxRenderTarget &renderTarget,
		const ur_float4x4 &viewProj, const ur_float3 &cameraPos)
	{
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
		res &= gfxContext.UpdateBuffer(this->gfxObjects.lightShaftsCB.get(), GfxGPUAccess::WriteDiscard, false, &cbResData, 0, cbResData.RowPitch);
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
	}

	void Atmosphere::ShowImgui()
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