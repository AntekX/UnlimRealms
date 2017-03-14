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

	Result Atmosphere::Init()
	{
		Result res(Success);

		res = this->CreateGfxObjects();

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

	Result Atmosphere::Render(GfxContext &gfxContext, const ur_float4x4 &viewProj)
	{
		// TODO:
		return NotImplemented;
	}

} // end namespace UnlimRealms