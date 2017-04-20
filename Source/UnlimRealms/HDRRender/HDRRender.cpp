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

		res = this->GetRealm().GetGfxSystem()->CreateRenderTarget(gfxObjects->hdrRT);
		if (Failed(res))
			return ResultError(Failure, "HDRRender::CreateGfxObjects: failed to create HDR render target");

		res = this->GetRealm().GetGfxSystem()->CreateRenderTarget(gfxObjects->luminanceRT);
		if (Failed(res))
			return ResultError(Failure, "HDRRender::CreateGfxObjects: failed to create luminance render target");

		// todo: create other gfx objects

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

		// (re)init luminance render targets chain
		ur_float widthLevels = floor(log2(ur_float(width)));
		ur_float heightLevels = floor(log2(ur_float(height)));
		descRT.Levels = 1;
		descRT.Width = (ur_uint)pow(2.0f, widthLevels)/4;
		descRT.Height = (ur_uint)pow(2.0f, heightLevels)/4;
		res = gfxObjects->luminanceRT->Initialize(descRT, false, GfxFormat::Unknown);
		if (Failed(res))
			return ResultError(Failure, "HDRRender::Init: failed to initialize luminance render targets chain");

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
		if (ur_null == this->gfxObjects || ur_null == genericRender)
			return NotInitialized;

		// render test luminance texture
		Result res = gfxContext.SetRenderTarget(this->gfxObjects->luminanceRT.get());
		if (Failed(res))
			return res;

		res = genericRender->RenderScreenQuad(gfxContext, this->gfxObjects->hdrRT->GetTargetBuffer());

		// todo: compute luminance targets chain

		return res;
	}

	Result HDRRender::Resolve(GfxContext &gfxContext)
	{
		GenericRender *genericRender = this->GetRealm().GetComponent<GenericRender>();
		if (ur_null == this->gfxObjects || ur_null == genericRender)
			return NotInitialized;

		// todo: render hdrRT texture to active RT using previously computed avg log luminance 
		Result res = genericRender->RenderScreenQuad(gfxContext, this->gfxObjects->hdrRT->GetTargetBuffer());

		return res;
	}

} // end namespace UnlimRealms