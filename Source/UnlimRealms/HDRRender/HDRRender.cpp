///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "HDRRender/HDRRender.h"
#include "Sys/Log.h"
#include "Resources/Resources.h"
#include "GenericRender/GenericRender.h"
#include "ImguiRender/ImguiRender.h"

namespace UnlimRealms
{

	HDRRender::HDRRender(Realm &realm) :
		RealmEntity(realm)
	{
	}

	HDRRender::~HDRRender()
	{

	}

	Result HDRRender::CreateGfxObjects()
	{
		Result res = Success;

		this->gfxObjects.reset(ur_null);
		std::unique_ptr<GfxObjects> gfxObjects(new GfxObjects());

		// HDR RT
		res = this->GetRealm().GetGfxSystem()->CreateRenderTarget(gfxObjects->hdrRT);
		if (Failed(res))
			return ResultError(Failure, "HDRRender::CreateGfxObjects: failed to create HDR render target");

		// bloom RT
		res = this->GetRealm().GetGfxSystem()->CreateRenderTarget(gfxObjects->bloomRT[0]);
		res &= this->GetRealm().GetGfxSystem()->CreateRenderTarget(gfxObjects->bloomRT[1]);
		if (Failed(res))
			return ResultError(Failure, "HDRRender::CreateGfxObjects: failed to create bloom render target");

		// constants CB
		res = this->GetRealm().GetGfxSystem()->CreateBuffer(gfxObjects->constantsCB);
		if (Succeeded(res))
		{
			res = gfxObjects->constantsCB->Initialize(sizeof(ConstantsCB), GfxUsage::Dynamic,
				(ur_uint)GfxBindFlag::ConstantBuffer, (ur_uint)GfxAccessFlag::Write);
		}
		if (Failed(res))
			return ResultError(Failure, "HDRRender::CreateGfxObjects: failed to initialize average luminance CB");

		// HDR target luminance PS
		res = CreatePixelShaderFromFile(this->GetRealm(), gfxObjects->HDRTargetLuminancePS, "HDRTargetLuminance_ps.cso");
		if (Failed(res))
			return ResultError(Failure, "HDRRender::CreateGfxObjects: failed to initialize HDR target luminance PS");

		// bloom luminance PS
		res = CreatePixelShaderFromFile(this->GetRealm(), gfxObjects->bloomLuminancePS, "BloomLuminance_ps.cso");
		if (Failed(res))
			return ResultError(Failure, "HDRRender::CreateGfxObjects: failed to initialize bloom luminance PS");

		// average luminance PS
		res = CreatePixelShaderFromFile(this->GetRealm(), gfxObjects->averageLuminancePS, "AverageLuminance_ps.cso");
		if (Failed(res))
			return ResultError(Failure, "HDRRender::CreateGfxObjects: failed to initialize average luminance PS");

		// tone mapping PS
		res = CreatePixelShaderFromFile(this->GetRealm(), gfxObjects->toneMappingPS, "ToneMapping_ps.cso");
		if (Failed(res))
			return ResultError(Failure, "HDRRender::CreateGfxObjects: failed to initialize tone mapping PS");

		// blur PS
		res = CreatePixelShaderFromFile(this->GetRealm(), gfxObjects->blurPS, "Blur_ps.cso");
		if (Failed(res))
			return ResultError(Failure, "HDRRender::CreateGfxObjects: failed to initialize blur PS");

		this->gfxObjects = std::move(gfxObjects);

		return res;
	}

	Result HDRRender::Init(ur_uint width, ur_uint height, bool depthStencilEnabled)
	{
		Result res = Success;

		if (ur_null == this->gfxObjects)
		{
			res = this->CreateGfxObjects();
			if (Failed(res))
				return res;
		}

		if (ur_null == this->gfxObjects)
			return NotInitialized;		

		// (re)init HDR target
		GfxTextureDesc descRT;
		descRT.Width = width;
		descRT.Height = height;
		descRT.Levels = 1;
		descRT.Format = GfxFormat::R16G16B16A16;
		descRT.FormatView = GfxFormatView::Float;
		descRT.Usage = GfxUsage::Default;
		descRT.BindFlags = ur_uint(GfxBindFlag::RenderTarget) | ur_uint(GfxBindFlag::ShaderResource);
		descRT.AccessFlags = ur_uint(0);
		res = gfxObjects->hdrRT->Initialize(descRT, depthStencilEnabled, GfxFormat::R24G8);
		if (Failed(res))
			return ResultError(Failure, "HDRRender::Init: failed to initialize HDR render target");

		// (re)init bloom target
		descRT.Format = GfxFormat::R16;
		descRT.Width = std::max(ur_uint(1), width / 4);
		descRT.Height = std::max(ur_uint(1), height / 4);
		res = gfxObjects->bloomRT[0]->Initialize(descRT, false, GfxFormat::Unknown);
		res &= gfxObjects->bloomRT[1]->Initialize(descRT, false, GfxFormat::Unknown);
		if (Failed(res))
			return ResultError(Failure, "HDRRender::Init: failed to initialize bloom render target");

		// (re)create and/or (re)init luminance render targets chain
		ur_float widthLevels = ceil(log2(ur_float(width)));
		ur_float heightLevels = ceil(log2(ur_float(height)));
		ur_uint lumRTChainSize = std::max(std::max(ur_uint(widthLevels), ur_uint(heightLevels)), ur_uint(1));
		descRT.Format = GfxFormat::R16;
		descRT.Width = (ur_uint)pow(2.0f, widthLevels - 1);
		descRT.Height = (ur_uint)pow(2.0f, heightLevels - 1);
		this->gfxObjects->lumRTChain.resize(lumRTChainSize);
		for (ur_uint i = 0; i < lumRTChainSize; ++i)
		{
			auto &avgLumRT = this->gfxObjects->lumRTChain[i];
			if (ur_null == avgLumRT.get())
			{
				res = this->GetRealm().GetGfxSystem()->CreateRenderTarget(avgLumRT);
				if (Failed(res)) break;
			}
			
			res = avgLumRT->Initialize(descRT, false, GfxFormat::Unknown);
			if (Failed(res)) break;

			descRT.Width = std::max(ur_uint(1), descRT.Width >> 1);
			descRT.Height = std::max(ur_uint(1), descRT.Height >> 1);
		}
		if (Failed(res))
		{
			this->gfxObjects->lumRTChain.clear();
			return ResultError(Failure, "HDRRender::Init: failed to initialize luminance render targets chain");
		}

		// init custom generic quad render state
		GenericRender *genericRender = this->GetRealm().GetComponent<GenericRender>();
		res = (genericRender != ur_null);
		if (Succeeded(res))
		{
			this->gfxObjects->quadPointSamplerRS = genericRender->GetDefaultQuadRenderState();
			this->gfxObjects->quadPointSamplerRS.SamplerState[0].MinFilter = GfxFilter::Point;
			this->gfxObjects->quadPointSamplerRS.SamplerState[0].MagFilter = GfxFilter::Point;
			this->gfxObjects->quadPointSamplerRS.SamplerState[1].MinFilter = GfxFilter::Linear;
			this->gfxObjects->quadPointSamplerRS.SamplerState[1].MagFilter = GfxFilter::Linear;
		}
		if (Failed(res))
			return ResultError(NotInitialized, "HDRRender::Init: failed to get GenericRender component");

		return res;
	}

	Result HDRRender::BeginRender(GfxContext &gfxContext)
	{
		if (ur_null == this->gfxObjects)
			return NotInitialized;

		bool hasDepthStencil = (this->gfxObjects->hdrRT->GetDepthStencilBuffer() != ur_null);
		
		Result res = gfxContext.SetRenderTarget(this->gfxObjects->hdrRT.get());
		if (Succeeded(res))
		{
			res = gfxContext.ClearTarget(this->gfxObjects->hdrRT.get(),
				true, { 0.0f, 0.0f, 0.0f, 0.0f },
				hasDepthStencil, 1.0f,
				hasDepthStencil, 0);
		}

		return res;
	}

	Result HDRRender::EndRender(GfxContext &gfxContext)
	{
		GenericRender *genericRender = this->GetRealm().GetComponent<GenericRender>();
		if (ur_null == this->gfxObjects || ur_null == genericRender ||
			this->gfxObjects->lumRTChain.empty())
			return NotInitialized;
		
		ConstantsCB cb;
		GfxResourceData cbResData = { &cb, sizeof(ConstantsCB), 0 };
		cb.SrcTargetSize.x = (ur_float)this->gfxObjects->hdrRT->GetTargetBuffer()->GetDesc().Width;
		cb.SrcTargetSize.y = (ur_float)this->gfxObjects->hdrRT->GetTargetBuffer()->GetDesc().Height;
		gfxContext.UpdateBuffer(this->gfxObjects->constantsCB.get(), GfxGPUAccess::WriteDiscard, false, &cbResData, 0, cbResData.RowPitch);

		// HDR RT to luminance first target
		Result res = Success;
		res &= gfxContext.SetRenderTarget(this->gfxObjects->lumRTChain[0].get());
		res &= genericRender->RenderScreenQuad(gfxContext, this->gfxObjects->hdrRT->GetTargetBuffer(), ur_null, ur_null,
			this->gfxObjects->HDRTargetLuminancePS.get(),
			this->gfxObjects->constantsCB.get());

		// compute average luminance from 2x2 texels of source RT and write to the next target in chain
		for (ur_size irt = 1; irt < this->gfxObjects->lumRTChain.size(); ++irt)
		{
			auto &srcRT = this->gfxObjects->lumRTChain[irt - 1];
			auto &dstRT = this->gfxObjects->lumRTChain[irt];

			cb.SrcTargetSize.x = (ur_float)srcRT->GetTargetBuffer()->GetDesc().Width;
			cb.SrcTargetSize.y = (ur_float)srcRT->GetTargetBuffer()->GetDesc().Height;
			gfxContext.UpdateBuffer(this->gfxObjects->constantsCB.get(), GfxGPUAccess::WriteDiscard, false, &cbResData, 0, cbResData.RowPitch);
			
			gfxContext.SetRenderTarget(dstRT.get());
			res = genericRender->RenderScreenQuad(gfxContext, srcRT->GetTargetBuffer(), ur_null, ur_null,
				this->gfxObjects->averageLuminancePS.get(),
				this->gfxObjects->constantsCB.get());
		}

		// compute bloom texture from HDR source
		gfxContext.SetRenderTarget(this->gfxObjects->bloomRT[0].get());
		res = genericRender->RenderScreenQuad(gfxContext, this->gfxObjects->hdrRT->GetTargetBuffer(), ur_null, ur_null,
			this->gfxObjects->bloomLuminancePS.get(),
			this->gfxObjects->constantsCB.get());
		
		// apply blur to bloom texture
		cb.SrcTargetSize.x = (ur_float)this->gfxObjects->bloomRT[0]->GetTargetBuffer()->GetDesc().Width;
		cb.SrcTargetSize.y = (ur_float)this->gfxObjects->bloomRT[0]->GetTargetBuffer()->GetDesc().Height;
		const ur_uint blurPasses = 4;
		ur_uint srcIdx = 0;
		ur_uint dstIdx = 0;
		for (ur_uint ipass = 0; ipass < blurPasses * 2; ++ipass, ++srcIdx)
		{
			cb.BlurDirection = ur_float(ipass % blurPasses);
			gfxContext.UpdateBuffer(this->gfxObjects->constantsCB.get(), GfxGPUAccess::WriteDiscard, false, &cbResData, 0, cbResData.RowPitch);

			srcIdx = srcIdx % 2;
			dstIdx = (srcIdx + 1) % 2;
			gfxContext.SetRenderTarget(this->gfxObjects->bloomRT[dstIdx].get());
			res = genericRender->RenderScreenQuad(gfxContext, this->gfxObjects->bloomRT[srcIdx]->GetTargetBuffer(), ur_null,
				&this->gfxObjects->quadPointSamplerRS,
				this->gfxObjects->blurPS.get(),
				this->gfxObjects->constantsCB.get());
		}
		if (dstIdx != 0) this->gfxObjects->bloomRT[0].swap(this->gfxObjects->bloomRT[1]);

		return res;
	}

	Result HDRRender::Resolve(GfxContext &gfxContext)
	{
		GenericRender *genericRender = this->GetRealm().GetComponent<GenericRender>();
		if (ur_null == this->gfxObjects || ur_null == genericRender ||
			this->gfxObjects->lumRTChain.empty())
			return NotInitialized;

		// temp: add debug control for white point and luminance scale
		static float DbgLumKey = 0.36f;
		static float DbgLumWhite = 1.0e+4f; // "infinite" white
		ImGui::Begin("HDR Rendering");
		ImGui::DragFloat("LumKey", &DbgLumKey, 0.01f, 0.01f, 1.0f);
		ImGui::InputFloat("LumWhite", &DbgLumWhite);
		ImGui::End();

		ConstantsCB cb;
		GfxResourceData cbResData = { &cb, sizeof(ConstantsCB), 0 };
		cb.SrcTargetSize.x = (ur_float)this->gfxObjects->hdrRT->GetTargetBuffer()->GetDesc().Width;
		cb.SrcTargetSize.y = (ur_float)this->gfxObjects->hdrRT->GetTargetBuffer()->GetDesc().Height;
		cb.LumKey = DbgLumKey;
		cb.LumWhite = DbgLumWhite;
		gfxContext.UpdateBuffer(this->gfxObjects->constantsCB.get(), GfxGPUAccess::WriteDiscard, false, &cbResData, 0, cbResData.RowPitch);

		// do tonemapping
		gfxContext.SetTexture(this->gfxObjects->lumRTChain.back()->GetTargetBuffer(), 1);
		gfxContext.SetTexture(this->gfxObjects->bloomRT[0]->GetTargetBuffer(), 2);
		Result res = genericRender->RenderScreenQuad(gfxContext, this->gfxObjects->hdrRT->GetTargetBuffer(), ur_null,
			&this->gfxObjects->quadPointSamplerRS,
			this->gfxObjects->toneMappingPS.get(),
			this->gfxObjects->constantsCB.get());

		// debug output
		#if 1
		auto &dbgRT = this->gfxObjects->bloomRT[0];
		//auto &dbgRT = this->gfxObjects->lumRTChain.back();
		GfxViewPort viewPort;
		gfxContext.GetViewPort(viewPort);
		ur_float sh = (ur_float)viewPort.Width / 8;
		ur_float w = (ur_float)dbgRT->GetTargetBuffer()->GetDesc().Width;
		ur_float h = (ur_float)dbgRT->GetTargetBuffer()->GetDesc().Height;
		genericRender->RenderScreenQuad(gfxContext, dbgRT->GetTargetBuffer(),
			RectF(0.0f, viewPort.Height - sh, sh * w / h, (ur_float)viewPort.Height));
		#endif

		return res;
	}

} // end namespace UnlimRealms