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

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Atmosphere
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Atmosphere::Atmosphere(Realm &realm) :
		RealmEntity(realm)
	{
	}

	Atmosphere::~Atmosphere()
	{
	}

	Result Atmosphere::Init(ur_float radius)
	{
		Result res(Success);

		res = this->CreateGfxObjects();

		if (Succeeded(res))
		{
			res = this->CreateMesh(radius);
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
			gfxRS.BlendState[0].DstBlend = GfxBlendFactor::InvSrcAlpha;
			gfxRS.BlendState[0].DstBlendAlpha = GfxBlendFactor::InvSrcAlpha;
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

		return res;
	}

	Result Atmosphere::CreateMesh(ur_float radius)
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
			y = radius * sin(ay);
			for (ur_uint ix = 0; ix < gridDetail; ++ix, ++pv)
			{
				ax = ur_float(ix) / gridDetail * MathConst<float>::Pi * 2.0f;
				r = radius * cos(ay);
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

		// view port
		const RectI &canvasBound = this->GetRealm().GetCanvas()->GetBound();
		GfxViewPort viewPort = { 0.0f, 0.0f, (float)canvasBound.Width(), (float)canvasBound.Height(), 0.0f, 1.0f };
		gfxContext.SetViewPort(&viewPort);
		
		// constants
		CommonCB cb;
		cb.viewProj = viewProj;
		cb.cameraPos = cameraPos;
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

} // end namespace UnlimRealms