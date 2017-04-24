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

		// constants CB
		res = this->GetRealm().GetGfxSystem()->CreateBuffer(gfxObjects->constantsCB);
		if (Succeeded(res))
		{
			res = gfxObjects->constantsCB->Initialize(sizeof(ConstantsCB), GfxUsage::Dynamic,
				(ur_uint)GfxBindFlag::ConstantBuffer, (ur_uint)GfxAccessFlag::Write);
		}
		if (Failed(res))
			return ResultError(Failure, "HDRRender::CreateGfxObjects: failed to initialize average luminance CB");

		// average luminance PS
		res = CreatePixelShaderFromFile(this->GetRealm(), gfxObjects->averageLuminancePS, "AverageLuminance_ps.cso");
		if (Failed(res))
			return ResultError(Failure, "HDRRender::CreateGfxObjects: failed to initialize average luminance PS");

		// tone mapping PS
		res = CreatePixelShaderFromFile(this->GetRealm(), gfxObjects->toneMappingPS, "ToneMapping_ps.cso");
		if (Failed(res))
			return ResultError(Failure, "HDRRender::CreateGfxObjects: failed to initialize tone mapping PS");

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

		// (re)create and/or (re)init luminance render targets chain
		ur_float widthLevels = ceil(log2(ur_float(width)));
		ur_float heightLevels = ceil(log2(ur_float(height)));
		ur_uint avgLumRTChainSize = std::max(std::max(ur_uint(widthLevels), ur_uint(heightLevels)), ur_uint(1));
		descRT.Levels = 1;
		descRT.Width = (ur_uint)pow(2.0f, widthLevels - 1);
		descRT.Height = (ur_uint)pow(2.0f, heightLevels - 1);
		this->gfxObjects->avgLumRTChain.resize(avgLumRTChainSize);
		for (ur_uint i = 0; i < avgLumRTChainSize; ++i)
		{
			auto &avgLumRT = this->gfxObjects->avgLumRTChain[i];
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
			this->gfxObjects->avgLumRTChain.clear();
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
			this->gfxObjects->avgLumRTChain.empty())
			return NotInitialized;
		
		ConstantsCB cb;
		GfxResourceData cbResData = { &cb, sizeof(ConstantsCB), 0 };
		cb.SrcTargetSize.x = (ur_float)this->gfxObjects->hdrRT->GetTargetBuffer()->GetDesc().Width;
		cb.SrcTargetSize.y = (ur_float)this->gfxObjects->hdrRT->GetTargetBuffer()->GetDesc().Height;
		gfxContext.UpdateBuffer(this->gfxObjects->constantsCB.get(), GfxGPUAccess::WriteDiscard, false, &cbResData, 0, cbResData.RowPitch);

		// HDR RT to luminance first target
		Result res = Success;
		res &= gfxContext.SetRenderTarget(this->gfxObjects->avgLumRTChain[0].get());
		res &= genericRender->RenderScreenQuad(gfxContext, this->gfxObjects->hdrRT->GetTargetBuffer(), ur_null,
			&this->gfxObjects->quadPointSamplerRS,
			this->gfxObjects->averageLuminancePS.get(),
			this->gfxObjects->constantsCB.get());

		// compute average luminance from 2x2 texels of source RT and write to the next target in chain
		for (ur_size irt = 1; irt < this->gfxObjects->avgLumRTChain.size(); ++irt)
		{
			auto &srcRT = this->gfxObjects->avgLumRTChain[irt - 1];
			auto &dstRT = this->gfxObjects->avgLumRTChain[irt];

			cb.SrcTargetSize.x = (ur_float)srcRT->GetTargetBuffer()->GetDesc().Width;
			cb.SrcTargetSize.y = (ur_float)srcRT->GetTargetBuffer()->GetDesc().Height;
			gfxContext.UpdateBuffer(this->gfxObjects->constantsCB.get(), GfxGPUAccess::WriteDiscard, false, &cbResData, 0, cbResData.RowPitch);
			
			gfxContext.SetRenderTarget(dstRT.get());
			res = genericRender->RenderScreenQuad(gfxContext, srcRT->GetTargetBuffer(), ur_null,
				&this->gfxObjects->quadPointSamplerRS,
				this->gfxObjects->averageLuminancePS.get(),
				this->gfxObjects->constantsCB.get());
		}

		return res;
	}

	Result HDRRender::Resolve(GfxContext &gfxContext)
	{
		GenericRender *genericRender = this->GetRealm().GetComponent<GenericRender>();
		if (ur_null == this->gfxObjects || ur_null == genericRender ||
			this->gfxObjects->avgLumRTChain.empty())
			return NotInitialized;

		ConstantsCB cb;
		GfxResourceData cbResData = { &cb, sizeof(ConstantsCB), 0 };
		cb.SrcTargetSize.x = (ur_float)this->gfxObjects->hdrRT->GetTargetBuffer()->GetDesc().Width;
		cb.SrcTargetSize.y = (ur_float)this->gfxObjects->hdrRT->GetTargetBuffer()->GetDesc().Height;
		cb.LumScale = 1.0f;
		cb.WhitePoint = 1.0f;
		// todo: add debug control for white point and luminance scale
		gfxContext.UpdateBuffer(this->gfxObjects->constantsCB.get(), GfxGPUAccess::WriteDiscard, false, &cbResData, 0, cbResData.RowPitch);

		// do tonemapping
		gfxContext.SetTexture(this->gfxObjects->avgLumRTChain.back()->GetTargetBuffer(), 1);
		Result res = genericRender->RenderScreenQuad(gfxContext, this->gfxObjects->hdrRT->GetTargetBuffer(), ur_null,
			&this->gfxObjects->quadPointSamplerRS,
			this->gfxObjects->toneMappingPS.get(),
			this->gfxObjects->constantsCB.get());

		{ // debug output
			auto &lumRT = this->gfxObjects->avgLumRTChain.back();
			GfxViewPort viewPort;
			gfxContext.GetViewPort(viewPort);
			ur_float sh = (ur_float)viewPort.Width / 8;
			ur_float w = (ur_float)lumRT->GetTargetBuffer()->GetDesc().Width;
			ur_float h = (ur_float)lumRT->GetTargetBuffer()->GetDesc().Height;
			genericRender->RenderScreenQuad(gfxContext, lumRT->GetTargetBuffer(),
				RectF(0.0f, viewPort.Height - sh, sh * w / h, (ur_float)viewPort.Height));
		}

		return res;
	}

} // end namespace UnlimRealms