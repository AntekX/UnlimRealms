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

	const HDRRender::Params HDRRender::Params::Default = {
		0.36f,		// LumKey
		1.0e+4f,	// LumWhite ("infinite" white)
		1.0f,		// BloomThreshold
	};

	const ur_uint BlurPasses = 8;

	HDRRender::HDRRender(Realm &realm) :
		RealmEntity(realm)
	{
		this->params = Params::Default;
		this->debugRT = DebugRT_None;
	#if defined(UR_GRAF)
		this->grafRenderer = realm.GetComponent<GrafRenderer>();
	#endif
	}

	HDRRender::~HDRRender()
	{
	#if defined(UR_GRAF)
		if (this->grafObjects != ur_null)
		{
			this->grafRenderer->SafeDelete(this->grafObjects.release());
		}
		if (this->grafRTObjects != ur_null)
		{
			this->grafRenderer->SafeDelete(this->grafRTObjects.release());
		}
	#endif
	}

	Result HDRRender::SetParams(const Params &params)
	{
		this->params = params;
		return Success;
	}

#if !defined(UR_GRAF)
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
			res = gfxObjects->constantsCB->Initialize(sizeof(ConstantsCB), 0, GfxUsage::Dynamic,
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

		// init custom generic quad render states
		GenericRender *genericRender = this->GetRealm().GetComponent<GenericRender>();
		res = (genericRender != ur_null);
		if (Succeeded(res))
		{
			// common
			GfxRenderState quadPointSamplerRS = genericRender->GetDefaultQuadRenderState();
			quadPointSamplerRS.SamplerState[0].MinFilter = GfxFilter::Point;
			quadPointSamplerRS.SamplerState[0].MagFilter = GfxFilter::Point;
			quadPointSamplerRS.SamplerState[1].MinFilter = GfxFilter::Linear;
			quadPointSamplerRS.SamplerState[1].MagFilter = GfxFilter::Linear;

			// HDR to Luminance
			genericRender->CreateScreenQuadState(gfxObjects->screenQuadStateHDRLuminance,
				gfxObjects->HDRTargetLuminancePS.get());

			// average Luminance
			genericRender->CreateScreenQuadState(gfxObjects->screenQuadStateAverageLuminance,
				gfxObjects->averageLuminancePS.get(), &quadPointSamplerRS);

			// bloom
			genericRender->CreateScreenQuadState(gfxObjects->screenQuadStateBloom,
				gfxObjects->bloomLuminancePS.get());

			// blur
			genericRender->CreateScreenQuadState(gfxObjects->screenQuadStateBlur,
				gfxObjects->blurPS.get(), &quadPointSamplerRS);

			// tonemapping
			genericRender->CreateScreenQuadState(gfxObjects->screenQuadStateTonemapping,
				gfxObjects->toneMappingPS.get(), &quadPointSamplerRS);

			// debug
			genericRender->CreateScreenQuadState(gfxObjects->screenQuadStateDebug,
				ur_null, &quadPointSamplerRS);
		}
		if (Failed(res))
			return ResultError(NotInitialized, "HDRRender::Init: failed to get GenericRender component");

		this->gfxObjects = std::move(gfxObjects);

		return res;
	}
#endif

	Result HDRRender::Init(ur_uint width, ur_uint height, bool depthStencilEnabled)
	{
	#if defined(UR_GRAF)
		return Result(NotImplemented);
	#else
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
		descRT.Format = GfxFormat::R16G16B16A16;
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

		return res;
	#endif
	}

	Result HDRRender::BeginRender(GfxContext &gfxContext)
	{
	#if defined(UR_GRAF)
		return Result(NotImplemented);
	#else
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
	#endif
	}

	Result HDRRender::EndRender(GfxContext &gfxContext)
	{
	#if defined(UR_GRAF)
		return Result(NotImplemented);
	#else
		GenericRender *genericRender = this->GetRealm().GetComponent<GenericRender>();
		if (ur_null == this->gfxObjects || ur_null == genericRender ||
			this->gfxObjects->lumRTChain.empty())
			return NotInitialized;

		Result res = Success;

		ConstantsCB cb;
		GfxResourceData cbResData = { &cb, sizeof(ConstantsCB), 0 };
		cb.SrcTargetSize.x = (ur_float)this->gfxObjects->hdrRT->GetTargetBuffer()->GetDesc().Width;
		cb.SrcTargetSize.y = (ur_float)this->gfxObjects->hdrRT->GetTargetBuffer()->GetDesc().Height;
		cb.params = this->params;
		res &= gfxContext.UpdateBuffer(this->gfxObjects->constantsCB.get(), GfxGPUAccess::WriteDiscard, &cbResData, 0, cbResData.RowPitch);
		res &= gfxContext.SetConstantBuffer(this->gfxObjects->constantsCB.get(), 1);

		// HDR RT to luminance first target
		res &= gfxContext.SetRenderTarget(this->gfxObjects->lumRTChain[0].get());
		res &= genericRender->RenderScreenQuad(gfxContext, this->gfxObjects->hdrRT->GetTargetBuffer(), ur_null,
			this->gfxObjects->screenQuadStateHDRLuminance.get());

		// compute average luminance from 2x2 texels of source RT and write to the next target in chain
		for (ur_size irt = 1; irt < this->gfxObjects->lumRTChain.size(); ++irt)
		{
			auto &srcRT = this->gfxObjects->lumRTChain[irt - 1];
			auto &dstRT = this->gfxObjects->lumRTChain[irt];

			cb.SrcTargetSize.x = (ur_float)srcRT->GetTargetBuffer()->GetDesc().Width;
			cb.SrcTargetSize.y = (ur_float)srcRT->GetTargetBuffer()->GetDesc().Height;
			gfxContext.UpdateBuffer(this->gfxObjects->constantsCB.get(), GfxGPUAccess::WriteDiscard, &cbResData, 0, cbResData.RowPitch);

			gfxContext.SetRenderTarget(dstRT.get());
			res &= genericRender->RenderScreenQuad(gfxContext, srcRT->GetTargetBuffer(), ur_null,
				this->gfxObjects->screenQuadStateAverageLuminance.get());
		}

		// compute bloom texture from HDR source
		gfxContext.SetRenderTarget(this->gfxObjects->bloomRT[0].get());
		res &= genericRender->RenderScreenQuad(gfxContext, this->gfxObjects->hdrRT->GetTargetBuffer(), ur_null,
			this->gfxObjects->screenQuadStateBloom.get());

		// apply blur to bloom texture
		cb.SrcTargetSize.x = (ur_float)this->gfxObjects->bloomRT[0]->GetTargetBuffer()->GetDesc().Width;
		cb.SrcTargetSize.y = (ur_float)this->gfxObjects->bloomRT[0]->GetTargetBuffer()->GetDesc().Height;
		const ur_uint blurPasses = 8;
		ur_uint srcIdx = 0;
		ur_uint dstIdx = 0;
		for (ur_uint ipass = 0; ipass < blurPasses * 2; ++ipass, ++srcIdx)
		{
			cb.BlurDirection = floor(ur_float(ipass) / blurPasses);
			res &= gfxContext.UpdateBuffer(this->gfxObjects->constantsCB.get(), GfxGPUAccess::WriteDiscard, &cbResData, 0, cbResData.RowPitch);

			srcIdx = srcIdx % 2;
			dstIdx = (srcIdx + 1) % 2;
			res &= gfxContext.SetRenderTarget(this->gfxObjects->bloomRT[dstIdx].get());
			res &= genericRender->RenderScreenQuad(gfxContext, this->gfxObjects->bloomRT[srcIdx]->GetTargetBuffer(), ur_null,
				this->gfxObjects->screenQuadStateBlur.get());
		}
		if (dstIdx != 0) this->gfxObjects->bloomRT[0].swap(this->gfxObjects->bloomRT[1]);

		return res;
	#endif
	}

	Result HDRRender::Resolve(GfxContext &gfxContext)
	{
	#if defined(UR_GRAF)
		return Result(NotImplemented);
	#else
		GenericRender *genericRender = this->GetRealm().GetComponent<GenericRender>();
		if (ur_null == this->gfxObjects || ur_null == genericRender ||
			this->gfxObjects->lumRTChain.empty())
			return NotInitialized;

		Result res = Success;

		ConstantsCB cb;
		GfxResourceData cbResData = { &cb, sizeof(ConstantsCB), 0 };
		cb.SrcTargetSize.x = (ur_float)this->gfxObjects->lumRTChain.front()->GetTargetBuffer()->GetDesc().Width;
		cb.SrcTargetSize.y = (ur_float)this->gfxObjects->lumRTChain.front()->GetTargetBuffer()->GetDesc().Height;
		cb.params = this->params;
		res &= gfxContext.UpdateBuffer(this->gfxObjects->constantsCB.get(), GfxGPUAccess::WriteDiscard, &cbResData, 0, cbResData.RowPitch);
		res &= gfxContext.SetConstantBuffer(this->gfxObjects->constantsCB.get(), 1);

		// do tonemapping
		res &= gfxContext.SetTexture(this->gfxObjects->lumRTChain.back()->GetTargetBuffer(), 1);
		res &= gfxContext.SetTexture(this->gfxObjects->bloomRT[0]->GetTargetBuffer(), 2);
		res &= genericRender->RenderScreenQuad(gfxContext, this->gfxObjects->hdrRT->GetTargetBuffer(), ur_null,
			this->gfxObjects->screenQuadStateTonemapping.get());

		// debug render
		if (this->debugRT != DebugRT_None)
		{
			GfxTexture *dbgTex = ur_null;
			switch (this->debugRT)
			{
			case DebugRT_HDR: dbgTex = this->gfxObjects->hdrRT ? this->gfxObjects->hdrRT->GetTargetBuffer() : ur_null; break;
			case DebugRT_Bloom: dbgTex = this->gfxObjects->bloomRT[0] ? this->gfxObjects->bloomRT[0]->GetTargetBuffer() : ur_null; break;
			case DebugRT_LumFirst: dbgTex = this->gfxObjects->lumRTChain.front() ? this->gfxObjects->lumRTChain.front()->GetTargetBuffer() : ur_null; break;
			case DebugRT_LumLast: dbgTex = this->gfxObjects->lumRTChain.back() ? this->gfxObjects->lumRTChain.back()->GetTargetBuffer() : ur_null; break;
			}
			if (dbgTex != ur_null)
			{
				GfxViewPort viewPort;
				gfxContext.GetViewPort(viewPort);
				ur_float sh = (ur_float)viewPort.Width / 5;
				ur_float w = (ur_float)dbgTex->GetDesc().Width;
				ur_float h = (ur_float)dbgTex->GetDesc().Height;
				genericRender->RenderScreenQuad(gfxContext, dbgTex,
					RectF(0.0f, viewPort.Height - sh, sh * w / h, (ur_float)viewPort.Height),
					this->gfxObjects->screenQuadStateDebug.get());
			}
		}

		return res;
	#endif
	}

	#if defined(UR_GRAF)
	HDRRender::GrafObjects::GrafObjects(GrafSystem& grafSystem) :
		GrafEntity(grafSystem)
	{
	}
	HDRRender::GrafObjects::~GrafObjects()
	{
	}
	HDRRender::GrafRTObjects::GrafRTObjects(GrafSystem& grafSystem) :
		GrafEntity(grafSystem)
	{
	}
	HDRRender::GrafRTObjects::~GrafRTObjects()
	{
	}
	#endif

	Result HDRRender::Init(ur_uint width, ur_uint height, GrafImage* depthStnecilRTImage)
	{
	#if !defined(UR_GRAF)
		return Result(NotImplemented);
	#else
		Result res = Success;

		if (ur_null == this->grafRenderer || ur_null == this->grafRenderer->GetGrafSystem())
			return Result(NotInitialized);

		GrafSystem* grafSystem = this->grafRenderer->GetGrafSystem();
		GrafDevice* grafDevice = this->grafRenderer->GetGrafDevice();
		ur_uint frameCount = this->grafRenderer->GetRecordedFrameCount();

		if (ur_null == this->grafObjects)
		{
			// create permanent GRAF objects

			std::unique_ptr<GrafObjects> grafObjects(new GrafObjects(*this->grafRenderer->GetGrafSystem()));

			// shaders

			res = GrafUtils::CreateShaderFromFile(*grafDevice, "FullScreenQuad_vs.spv", GrafShaderType::Vertex, grafObjects->fullScreenQuadVS);
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to create shader for FullScreenQuad_vs");

			res = GrafUtils::CreateShaderFromFile(*grafDevice, "HDRTargetLuminance_ps.spv", GrafShaderType::Pixel, grafObjects->luminancePS);
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to create shader for HDRTargetLuminance_ps");

			res = GrafUtils::CreateShaderFromFile(*grafDevice, "AverageLuminance_ps.spv", GrafShaderType::Pixel, grafObjects->luminanceAvgPS);
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to create shader for AverageLuminance_ps");

			res = GrafUtils::CreateShaderFromFile(*grafDevice, "BloomLuminance_ps.spv", GrafShaderType::Pixel, grafObjects->bloomPS);
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to create shader for BloomLuminance_ps");

			res = GrafUtils::CreateShaderFromFile(*grafDevice, "Blur_ps.spv", GrafShaderType::Pixel, grafObjects->blurPS);
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to create shader for Blur_ps");

			res = GrafUtils::CreateShaderFromFile(*grafDevice, "ToneMapping_ps.spv", GrafShaderType::Pixel, grafObjects->tonemapPS);
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to create shader for ToneMapping_ps");

			// luminance descriptors layout

			res = grafSystem->CreateDescriptorTableLayout(grafObjects->luminancePSDescLayout);
			if (Succeeded(res))
			{
				GrafDescriptorRangeDesc grafDescriptorRanges[] = {
					{ GrafDescriptorType::ConstantBuffer, 1, 1 },
					{ GrafDescriptorType::Sampler, 0, 1 },
					{ GrafDescriptorType::Texture, 0, 1 }
				};
				GrafDescriptorTableLayoutDesc grafDescriptorLayoutDesc = {
					GrafShaderStageFlags((ur_uint)GrafShaderStageFlag::Pixel),
					grafDescriptorRanges, ur_array_size(grafDescriptorRanges)
				};
				res = grafObjects->luminancePSDescLayout->Initialize(grafDevice, { grafDescriptorLayoutDesc });
			}
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to initialize luminance descriptors layout");

			// tonemapping descriptors layout

			res = grafSystem->CreateDescriptorTableLayout(grafObjects->tonemapPSDescLayout);
			if (Succeeded(res))
			{
				GrafDescriptorRangeDesc grafDescriptorRanges[] = {
					{ GrafDescriptorType::ConstantBuffer, 1, 1 },
					{ GrafDescriptorType::Sampler, 0, 2 },
					{ GrafDescriptorType::Texture, 0, 3 }
				};
				GrafDescriptorTableLayoutDesc grafDescriptorLayoutDesc = {
					GrafShaderStageFlags((ur_uint)GrafShaderStageFlag::Pixel),
					grafDescriptorRanges, ur_array_size(grafDescriptorRanges)
				};
				res = grafObjects->tonemapPSDescLayout->Initialize(grafDevice, { grafDescriptorLayoutDesc });
			}
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to initialize tonemapping descriptors layout");

			// tonemapping descriptor tables

			ur_size tonemapTableCount = this->grafRenderer->GetRecordedFrameCount();
			grafObjects->tonemapPSDescTables.resize(tonemapTableCount);
			for (ur_uint i = 0; i < tonemapTableCount; ++i)
			{
				res = grafSystem->CreateDescriptorTable(grafObjects->tonemapPSDescTables[i]);
				if (Succeeded(res))
				{
					res = grafObjects->tonemapPSDescTables[i]->Initialize(grafDevice, { grafObjects->tonemapPSDescLayout.get() });
				}
				if (Failed(res))
					break;
			}
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to initialize tonemapping decsriptor table(s)");

			// bloom & blur descriptor tables

			ur_size bloomTableCount = this->grafRenderer->GetRecordedFrameCount() * (BlurPasses * 2 + 1);
			grafObjects->bloomPSDescTables.resize(bloomTableCount);
			for (ur_uint i = 0; i < bloomTableCount; ++i)
			{
				res = grafSystem->CreateDescriptorTable(grafObjects->bloomPSDescTables[i]);
				if (Succeeded(res))
				{
					res = grafObjects->bloomPSDescTables[i]->Initialize(grafDevice, { grafObjects->luminancePSDescLayout.get() });
				}
				if (Failed(res))
					break;
			}
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to initialize bloom decsriptor table(s)");

			// HDR render pass

			res = grafSystem->CreateRenderPass(grafObjects->hdrRenderPass);
			if (Succeeded(res))
			{
				GrafRenderPassImageDesc hdrRenderPassDesc[] = {
					{ // color
						GrafFormat::R16G16B16A16_SFLOAT,
						GrafImageState::Undefined, GrafImageState::ColorWrite,
						GrafRenderPassDataOp::Clear, GrafRenderPassDataOp::Store,
						GrafRenderPassDataOp::DontCare, GrafRenderPassDataOp::DontCare
					},
					{ // depth & stencil
						GrafFormat::D24_UNORM_S8_UINT,
						GrafImageState::Undefined, GrafImageState::DepthStencilWrite,
						GrafRenderPassDataOp::Clear, GrafRenderPassDataOp::Store,
						GrafRenderPassDataOp::Clear, GrafRenderPassDataOp::Store
					}
				};
				res = grafObjects->hdrRenderPass->Initialize(grafDevice, { hdrRenderPassDesc, (depthStnecilRTImage ? 2u : 1u) });
			}
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to initialize HDR render pass");

			// luminance render pass

			res = grafSystem->CreateRenderPass(grafObjects->lumRenderPass);
			if (Succeeded(res))
			{
				GrafRenderPassImageDesc lumRenderPassDesc[] = {
					{
						GrafFormat::R16_SFLOAT,
						GrafImageState::Undefined, GrafImageState::ColorWrite,
						GrafRenderPassDataOp::DontCare, GrafRenderPassDataOp::Store,
						GrafRenderPassDataOp::DontCare, GrafRenderPassDataOp::DontCare
					}
				};
				res = grafObjects->lumRenderPass->Initialize(grafDevice, { lumRenderPassDesc, 1u });
			}
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to initialize luminance render pass");

			// bloom render pass

			res = grafSystem->CreateRenderPass(grafObjects->bloomRenderPass);
			if (Succeeded(res))
			{
				GrafRenderPassImageDesc lumRenderPassDesc[] = {
					{
						GrafFormat::R16G16B16A16_SFLOAT,
						GrafImageState::Undefined, GrafImageState::ColorWrite,
						GrafRenderPassDataOp::DontCare, GrafRenderPassDataOp::Store,
						GrafRenderPassDataOp::DontCare, GrafRenderPassDataOp::DontCare
					}
				};
				res = grafObjects->bloomRenderPass->Initialize(grafDevice, { lumRenderPassDesc, 1u });
			}
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to initialize bloom render pass");

			// tonemapping render pass

			res = grafSystem->CreateRenderPass(grafObjects->tonemapRenderPass);
			if (Succeeded(res))
			{
				GrafRenderPassImageDesc tonemapRenderPassDesc[] = {
					{ // color
						GrafFormat::B8G8R8A8_UNORM,
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
				res = grafObjects->tonemapRenderPass->Initialize(grafDevice, { tonemapRenderPassDesc, (depthStnecilRTImage ? 2u : 1u) });
			}
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to initialize tonemapping render pass");

			// pipeline config: calculate luminance

			res = grafSystem->CreatePipeline(grafObjects->lumPipeline);
			if (Succeeded(res))
			{
				GrafShader* shaderStages[] = {
					grafObjects->fullScreenQuadVS.get(),
					grafObjects->luminancePS.get()
				};
				GrafDescriptorTableLayout* descriptorLayouts[] = {
					grafObjects->luminancePSDescLayout.get(),
				};
				GrafPipeline::InitParams pipelineParams = GrafPipeline::InitParams::Default;
				pipelineParams.RenderPass = grafObjects->lumRenderPass.get();
				pipelineParams.ShaderStages = shaderStages;
				pipelineParams.ShaderStageCount = ur_array_size(shaderStages);
				pipelineParams.DescriptorTableLayouts = descriptorLayouts;
				pipelineParams.DescriptorTableLayoutCount = ur_array_size(descriptorLayouts);
				pipelineParams.PrimitiveTopology = GrafPrimitiveTopology::TriangleStrip;
				pipelineParams.FrontFaceOrder = GrafFrontFaceOrder::Clockwise;
				pipelineParams.CullMode = GrafCullMode::Back;
				res = grafObjects->lumPipeline->Initialize(grafDevice, pipelineParams);
			}
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to initialize luminance pipeline");

			// pipeline config: calculate average luminance

			res = grafSystem->CreatePipeline(grafObjects->avgPipeline);
			if (Succeeded(res))
			{
				GrafShader* shaderStages[] = {
					grafObjects->fullScreenQuadVS.get(),
					grafObjects->luminanceAvgPS.get()
				};
				GrafDescriptorTableLayout* descriptorLayouts[] = {
					grafObjects->luminancePSDescLayout.get(),
				};
				GrafPipeline::InitParams pipelineParams = GrafPipeline::InitParams::Default;
				pipelineParams.RenderPass = grafObjects->lumRenderPass.get();
				pipelineParams.ShaderStages = shaderStages;
				pipelineParams.ShaderStageCount = ur_array_size(shaderStages);
				pipelineParams.DescriptorTableLayouts = descriptorLayouts;
				pipelineParams.DescriptorTableLayoutCount = ur_array_size(descriptorLayouts);
				pipelineParams.PrimitiveTopology = GrafPrimitiveTopology::TriangleStrip;
				pipelineParams.FrontFaceOrder = GrafFrontFaceOrder::Clockwise;
				pipelineParams.CullMode = GrafCullMode::Back;
				res = grafObjects->avgPipeline->Initialize(grafDevice, pipelineParams);
			}
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to initialize average luminance pipeline");

			// pipeline config: calculate bloom luminance

			res = grafSystem->CreatePipeline(grafObjects->bloomPipeline);
			if (Succeeded(res))
			{
				GrafShader* shaderStages[] = {
					grafObjects->fullScreenQuadVS.get(),
					grafObjects->bloomPS.get()
				};
				GrafDescriptorTableLayout* descriptorLayouts[] = {
					grafObjects->luminancePSDescLayout.get(),
				};
				GrafPipeline::InitParams pipelineParams = GrafPipeline::InitParams::Default;
				pipelineParams.RenderPass = grafObjects->bloomRenderPass.get();
				pipelineParams.ShaderStages = shaderStages;
				pipelineParams.ShaderStageCount = ur_array_size(shaderStages);
				pipelineParams.DescriptorTableLayouts = descriptorLayouts;
				pipelineParams.DescriptorTableLayoutCount = ur_array_size(descriptorLayouts);
				pipelineParams.PrimitiveTopology = GrafPrimitiveTopology::TriangleStrip;
				pipelineParams.FrontFaceOrder = GrafFrontFaceOrder::Clockwise;
				pipelineParams.CullMode = GrafCullMode::Back;
				res = grafObjects->bloomPipeline->Initialize(grafDevice, pipelineParams);
			}
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to initialize bloom pipeline");

			// pipeline config: bloom blur filter

			res = grafSystem->CreatePipeline(grafObjects->blurPipeline);
			if (Succeeded(res))
			{
				GrafShader* shaderStages[] = {
					grafObjects->fullScreenQuadVS.get(),
					grafObjects->blurPS.get()
				};
				GrafDescriptorTableLayout* descriptorLayouts[] = {
					grafObjects->luminancePSDescLayout.get(),
				};
				GrafPipeline::InitParams pipelineParams = GrafPipeline::InitParams::Default;
				pipelineParams.RenderPass = grafObjects->bloomRenderPass.get();
				pipelineParams.ShaderStages = shaderStages;
				pipelineParams.ShaderStageCount = ur_array_size(shaderStages);
				pipelineParams.DescriptorTableLayouts = descriptorLayouts;
				pipelineParams.DescriptorTableLayoutCount = ur_array_size(descriptorLayouts);
				pipelineParams.PrimitiveTopology = GrafPrimitiveTopology::TriangleStrip;
				pipelineParams.FrontFaceOrder = GrafFrontFaceOrder::Clockwise;
				pipelineParams.CullMode = GrafCullMode::Back;
				res = grafObjects->blurPipeline->Initialize(grafDevice, pipelineParams);
			}
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to initialize blur pipeline");

			// pipeline config: tonemapping

			res = grafSystem->CreatePipeline(grafObjects->tonemapPipeline);
			if (Succeeded(res))
			{
				GrafShader* shaderStages[] = {
					grafObjects->fullScreenQuadVS.get(),
					grafObjects->tonemapPS.get()
				};
				GrafDescriptorTableLayout* descriptorLayouts[] = {
					grafObjects->tonemapPSDescLayout.get(),
				};
				GrafColorBlendOpDesc colorBlendOpDesc = {
					GrafColorChannelFlags(GrafColorChannelFlag::All), true,
					GrafBlendOp::Add, GrafBlendFactor::One, GrafBlendFactor::InvSrcAlpha,
					GrafBlendOp::Add, GrafBlendFactor::SrcAlpha, GrafBlendFactor::InvSrcAlpha,
				};
				GrafPipeline::InitParams pipelineParams = GrafPipeline::InitParams::Default;
				pipelineParams.RenderPass = grafObjects->tonemapRenderPass.get();
				pipelineParams.ShaderStages = shaderStages;
				pipelineParams.ShaderStageCount = ur_array_size(shaderStages);
				pipelineParams.DescriptorTableLayouts = descriptorLayouts;
				pipelineParams.DescriptorTableLayoutCount = ur_array_size(descriptorLayouts);
				pipelineParams.PrimitiveTopology = GrafPrimitiveTopology::TriangleStrip;
				pipelineParams.FrontFaceOrder = GrafFrontFaceOrder::Clockwise;
				pipelineParams.CullMode = GrafCullMode::Back;
				pipelineParams.ColorBlendOpDescCount = 1;
				pipelineParams.ColorBlendOpDesc = &colorBlendOpDesc;
				res = grafObjects->tonemapPipeline->Initialize(grafDevice, pipelineParams);
			}
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to initialize tonemapping pipeline");

			// samplers

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
				return ResultError(Failure, "HDRRender::Init: failed to initialize sampler");

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
				return ResultError(Failure, "HDRRender::Init: failed to initialize sampler");

			this->grafObjects.reset(grafObjects.release());
		}

		// (re)create RTs and images

		if (this->grafRTObjects != ur_null)
		{
			this->grafRenderer->SafeDelete(this->grafRTObjects.release());
		}
		
		std::unique_ptr<GrafRTObjects> grafRTObjects(new GrafRTObjects(*this->grafRenderer->GetGrafSystem()));

		// HDR target image

		res = grafSystem->CreateImage(grafRTObjects->hdrRTImage);
		if (Succeeded(res))
		{
			GrafImageDesc hdrRTImageDesc = {
				GrafImageType::Tex2D,
				GrafFormat::R16G16B16A16_SFLOAT,
				ur_uint3(width, height, 1), 1,
				ur_uint(GrafImageUsageFlag::ColorRenderTarget) | ur_uint(GrafImageUsageFlag::ShaderInput),
				ur_uint(GrafDeviceMemoryFlag::GpuLocal)
			};
			res = grafRTObjects->hdrRTImage->Initialize(grafDevice, { hdrRTImageDesc });
		}
		if (Failed(res))
			return ResultError(Failure, "HDRRender::Init: failed to initialize hdrRTImage");

		// HDR render target set

		res = grafSystem->CreateRenderTarget(grafRTObjects->hdrRT);
		if (Succeeded(res))
		{
			GrafImage* hdrRTImages[] = {
				grafRTObjects->hdrRTImage.get(),
				depthStnecilRTImage
			};
			res = grafRTObjects->hdrRT->Initialize(grafDevice, { grafObjects->hdrRenderPass.get(), hdrRTImages, (depthStnecilRTImage ? 2u : 1u) } );
		}
		if (Failed(res))
			return ResultError(Failure, "HDRRender::Init: failed to initialize hdrRT");

		// luminance images

		ur_float widthLevels = ceil(log2(ur_float(width)));
		ur_float heightLevels = ceil(log2(ur_float(height)));
		ur_uint lumRTChainSize = std::max(std::max(ur_uint(widthLevels), ur_uint(heightLevels)), ur_uint(1));
		GrafImageDesc lumRTImageDesc = {
			GrafImageType::Tex2D,
			GrafFormat::R16_SFLOAT,
			ur_uint3((ur_uint)pow(2.0f, widthLevels - 1), (ur_uint)pow(2.0f, heightLevels - 1), 1), 1,
			ur_uint(GrafImageUsageFlag::ColorRenderTarget) | ur_uint(GrafImageUsageFlag::ShaderInput),
			ur_uint(GrafDeviceMemoryFlag::GpuLocal)
		};
		grafRTObjects->lumRTChainImages.resize(lumRTChainSize);
		grafRTObjects->lumRTChain.resize(lumRTChainSize);
		for (ur_uint i = 0; i < lumRTChainSize; ++i)
		{
			auto &avgLumImage = grafRTObjects->lumRTChainImages[i];
			res = grafSystem->CreateImage(avgLumImage);
			if (Succeeded(res))
			{
				res = avgLumImage->Initialize(grafDevice, { lumRTImageDesc });
			}
			if (Failed(res))
				break;

			auto& avgLumRT = grafRTObjects->lumRTChain[i];
			res = grafSystem->CreateRenderTarget(avgLumRT);
			if (Succeeded(res))
			{
				GrafImage* lumRTImages[] = { avgLumImage.get() };
				res = avgLumRT->Initialize(grafDevice, { grafObjects->lumRenderPass.get(), lumRTImages, 1 });
			}
			if (Failed(res))
				break;
			
			lumRTImageDesc.Size.x = std::max(ur_uint(1), lumRTImageDesc.Size.x >> 1);
			lumRTImageDesc.Size.y = std::max(ur_uint(1), lumRTImageDesc.Size.y >> 1);
		}
		if (Failed(res))
			return ResultError(Failure, "HDRRender::Init: failed to initialize luminance image(s)");

		// luminance pass descriptor tables

		ur_size descTableCount = lumRTChainSize * this->grafRenderer->GetRecordedFrameCount();
		grafRTObjects->lumRTChainTables.resize(descTableCount);
		for (ur_uint i = 0; i < descTableCount; ++i)
		{
			res = grafSystem->CreateDescriptorTable(grafRTObjects->lumRTChainTables[i]);
			if (Succeeded(res))
			{
				res = grafRTObjects->lumRTChainTables[i]->Initialize(grafDevice, { this->grafObjects->luminancePSDescLayout.get() });
			}
			if (Failed(res))
				break;
		}
		if (Failed(res))
			return ResultError(Failure, "HDRRender::Init: failed to initialize decsriptor table(s)");

		// bloom image

		GrafImageDesc bloomImageDesc = {
			GrafImageType::Tex2D,
			GrafFormat::R16G16B16A16_SFLOAT,
			ur_uint3((ur_uint)std::max(ur_uint(1), width / 4), (ur_uint)std::max(ur_uint(1), height / 4), 1), 1,
			ur_uint(GrafImageUsageFlag::ColorRenderTarget) | ur_uint(GrafImageUsageFlag::ShaderInput),
			ur_uint(GrafDeviceMemoryFlag::GpuLocal)
		};
		for (ur_size iimg = 0; iimg < 2; ++iimg)
		{
			res = grafSystem->CreateImage(grafRTObjects->bloomImage[iimg]);
			if (Succeeded(res))
			{
				res = grafRTObjects->bloomImage[iimg]->Initialize(grafDevice, { bloomImageDesc });
			}
			if (Failed(res))
				break;
			
			res = grafSystem->CreateRenderTarget(grafRTObjects->bloomRT[iimg]);
			if (Succeeded(res))
			{
				GrafImage* bloomRTImages[] = {
					grafRTObjects->bloomImage[iimg].get(),
				};
				res = grafRTObjects->bloomRT[iimg]->Initialize(grafDevice, { grafObjects->bloomRenderPass.get(), bloomRTImages, 1u });
			}
		}
		if (Failed(res))
			return ResultError(Failure, "HDRRender::Init: failed to initialize bloom image(s)");

		this->grafRTObjects.reset(grafRTObjects.release());

		return res;
	#endif
	}

	Result HDRRender::BeginRender(GrafCommandList &grafCmdList)
	{
	#if !defined(UR_GRAF)
		return Result(NotImplemented);
	#else
		if (ur_null == this->grafRTObjects.get())
			return Result(NotInitialized);

		GrafViewportDesc rtViewport = {};
		rtViewport.Width = (ur_float)this->grafRTObjects->hdrRTImage->GetDesc().Size.x;
		rtViewport.Height = (ur_float)this->grafRTObjects->hdrRTImage->GetDesc().Size.y;
		rtViewport.Near = 0.0f;
		rtViewport.Far = 1.0f;
		grafCmdList.SetViewport(rtViewport, true);

		static GrafClearValue hdrRTClearValues[] = {
			{ 0.0f, 0.0f, 0.0f, 0.0f }, // color
			{ 1.0f, 0.0f, 0.0f, 0.0f }, // depth & stencil
		};
		grafCmdList.ImageMemoryBarrier(this->grafRTObjects->hdrRTImage.get(), GrafImageState::Current, GrafImageState::ColorWrite);
		grafCmdList.BeginRenderPass(this->grafObjects->hdrRenderPass.get(), this->grafRTObjects->hdrRT.get(), hdrRTClearValues);

		return Result(Success);
	#endif
	}

	Result HDRRender::EndRender(GrafCommandList &grafCmdList)
	{
	#if !defined(UR_GRAF)
		return Result(NotImplemented);
	#else
		if (ur_null == this->grafRTObjects.get())
			return Result(NotInitialized);

		grafCmdList.EndRenderPass();

		return Result(Success);
	#endif
	}

	Result HDRRender::Resolve(GrafCommandList &grafCmdList, GrafRenderTarget* resolveTarget)
	{
	#if !defined(UR_GRAF)
		return Result(NotImplemented);
	#else
		if (ur_null == this->grafRTObjects.get())
			return Result(NotInitialized);

		Result res = Success;
		ur_uint frameIdx = this->grafRenderer->GetCurrentFrameId();
		ur_uint frameFirstTableId = frameIdx * (ur_uint)this->grafRTObjects->lumRTChain.size();

		// HDR RT to first luminance image

		ConstantsCB cb;
		cb.SrcTargetSize.x = (ur_float)this->grafRTObjects->hdrRTImage->GetDesc().Size.x;
		cb.SrcTargetSize.y = (ur_float)this->grafRTObjects->hdrRTImage->GetDesc().Size.y;
		cb.params = this->params;
		GrafBuffer* dynamicCB = this->grafRenderer->GetDynamicConstantBuffer();
		Allocation dynamicCBAlloc = grafRenderer->GetDynamicConstantBufferAllocation(sizeof(ConstantsCB));
		dynamicCB->Write((ur_byte*)&cb, sizeof(cb), 0, dynamicCBAlloc.Offset);

		GrafDescriptorTable* descTable = this->grafRTObjects->lumRTChainTables[frameFirstTableId].get();
		descTable->SetConstantBuffer(1, dynamicCB, dynamicCBAlloc.Offset, dynamicCBAlloc.Size);
		descTable->SetSampler(0, this->grafObjects->samplerLinear.get());
		descTable->SetImage(0, this->grafRTObjects->hdrRTImage.get());

		GrafViewportDesc rtViewport = {};
		rtViewport.Width = (ur_float)this->grafRTObjects->lumRTChainImages[0]->GetDesc().Size.x;
		rtViewport.Height = (ur_float)this->grafRTObjects->lumRTChainImages[0]->GetDesc().Size.y;
		rtViewport.Near = 0.0f;
		rtViewport.Far = 1.0f;
		
		grafCmdList.SetViewport(rtViewport, true);
		grafCmdList.ImageMemoryBarrier(this->grafRTObjects->hdrRTImage.get(), GrafImageState::Current, GrafImageState::ShaderRead);
		grafCmdList.ImageMemoryBarrier(this->grafRTObjects->lumRTChainImages[0].get(), GrafImageState::Current, GrafImageState::ColorWrite);
		grafCmdList.BeginRenderPass(this->grafObjects->lumRenderPass.get(), this->grafRTObjects->lumRTChain[0].get());
		grafCmdList.BindPipeline(this->grafObjects->lumPipeline.get());
		grafCmdList.BindDescriptorTable(descTable, this->grafObjects->lumPipeline.get());
		grafCmdList.Draw(4, 1, 0, 0);
		grafCmdList.EndRenderPass();

		// compute average luminance from 2x2 texels of source RT and write to the next target in chain

		grafCmdList.BindPipeline(this->grafObjects->avgPipeline.get());
		for (ur_size irt = 1; irt < this->grafRTObjects->lumRTChain.size(); ++irt)
		{
			auto &srcRTImage = this->grafRTObjects->lumRTChainImages[irt - 1];
			auto &dstRTImage = this->grafRTObjects->lumRTChainImages[irt];

			cb.SrcTargetSize.x = (ur_float)srcRTImage->GetDesc().Size.x;
			cb.SrcTargetSize.y = (ur_float)srcRTImage->GetDesc().Size.y;
			dynamicCBAlloc = grafRenderer->GetDynamicConstantBufferAllocation(sizeof(ConstantsCB));
			dynamicCB->Write((ur_byte*)&cb, sizeof(cb), 0, dynamicCBAlloc.Offset);

			descTable = this->grafRTObjects->lumRTChainTables[frameFirstTableId + irt].get();
			descTable->SetConstantBuffer(1, dynamicCB, dynamicCBAlloc.Offset, dynamicCBAlloc.Size);
			descTable->SetSampler(0, this->grafObjects->samplerPoint.get());
			descTable->SetImage(0, srcRTImage.get());

			rtViewport.Width = (ur_float)dstRTImage->GetDesc().Size.x;
			rtViewport.Height = (ur_float)dstRTImage->GetDesc().Size.y;

			grafCmdList.SetViewport(rtViewport, true);
			grafCmdList.ImageMemoryBarrier(srcRTImage.get(), GrafImageState::Current, GrafImageState::ShaderRead);
			grafCmdList.ImageMemoryBarrier(dstRTImage.get(), GrafImageState::Current, GrafImageState::ColorWrite);
			grafCmdList.BeginRenderPass(this->grafObjects->lumRenderPass.get(), this->grafRTObjects->lumRTChain[irt].get());
			grafCmdList.BindDescriptorTable(this->grafRTObjects->lumRTChainTables[frameFirstTableId + irt].get(), this->grafObjects->avgPipeline.get());
			grafCmdList.Draw(4, 1, 0, 0);
			grafCmdList.EndRenderPass();
		}

		// compute bloom texture from HDR source
		
		rtViewport.Width = (ur_float)this->grafRTObjects->bloomImage[0]->GetDesc().Size.x;
		rtViewport.Height = (ur_float)this->grafRTObjects->bloomImage[0]->GetDesc().Size.y;
		
		cb.SrcTargetSize.x = (ur_float)rtViewport.Width;
		cb.SrcTargetSize.y = (ur_float)rtViewport.Height;
		dynamicCBAlloc = grafRenderer->GetDynamicConstantBufferAllocation(sizeof(ConstantsCB));
		dynamicCB->Write((ur_byte*)&cb, sizeof(cb), 0, dynamicCBAlloc.Offset);

		ur_size bloomTableId = frameIdx * (BlurPasses * 2 + 1);
		descTable = this->grafObjects->bloomPSDescTables[bloomTableId++].get();
		descTable->SetConstantBuffer(1, dynamicCB, dynamicCBAlloc.Offset, dynamicCBAlloc.Size);
		descTable->SetSampler(0, this->grafObjects->samplerPoint.get());
		descTable->SetImage(0, this->grafRTObjects->hdrRTImage.get());

		grafCmdList.SetViewport(rtViewport, true);
		grafCmdList.ImageMemoryBarrier(this->grafRTObjects->bloomImage[0].get(), GrafImageState::Current, GrafImageState::ColorWrite);
		grafCmdList.BeginRenderPass(this->grafObjects->bloomRenderPass.get(), this->grafRTObjects->bloomRT[0].get());
		grafCmdList.BindPipeline(this->grafObjects->bloomPipeline.get());
		grafCmdList.BindDescriptorTable(descTable, this->grafObjects->bloomPipeline.get());
		grafCmdList.Draw(4, 1, 0, 0);
		grafCmdList.EndRenderPass();

		// apply blur to bloom texture

		ur_uint srcIdx = 0;
		ur_uint dstIdx = 0;
		for (ur_uint ipass = 0; ipass < BlurPasses * 2; ++ipass, ++srcIdx)
		{
			srcIdx = srcIdx % 2;
			dstIdx = (srcIdx + 1) % 2;

			cb.BlurDirection = floor(ur_float(ipass) / BlurPasses);
			dynamicCBAlloc = grafRenderer->GetDynamicConstantBufferAllocation(sizeof(ConstantsCB));
			dynamicCB->Write((ur_byte*)&cb, sizeof(cb), 0, dynamicCBAlloc.Offset);

			descTable = this->grafObjects->bloomPSDescTables[bloomTableId++].get();
			descTable->SetConstantBuffer(1, dynamicCB, dynamicCBAlloc.Offset, dynamicCBAlloc.Size);
			descTable->SetSampler(0, this->grafObjects->samplerPoint.get());
			descTable->SetImage(0, this->grafRTObjects->bloomImage[srcIdx].get());
			
			grafCmdList.ImageMemoryBarrier(this->grafRTObjects->bloomImage[srcIdx].get(), GrafImageState::Current, GrafImageState::ShaderRead);
			grafCmdList.ImageMemoryBarrier(this->grafRTObjects->bloomImage[dstIdx].get(), GrafImageState::Current, GrafImageState::ColorWrite);
			grafCmdList.BeginRenderPass(this->grafObjects->bloomRenderPass.get(), this->grafRTObjects->bloomRT[dstIdx].get());
			grafCmdList.BindPipeline(this->grafObjects->blurPipeline.get());
			grafCmdList.BindDescriptorTable(descTable, this->grafObjects->blurPipeline.get());
			grafCmdList.Draw(4, 1, 0, 0);
			grafCmdList.EndRenderPass();
		}
		if (dstIdx != 0) this->grafRTObjects->bloomImage[0].swap(this->grafRTObjects->bloomImage[1]);

		// apply tonemapping and write to resolve target

		cb.SrcTargetSize.x = (ur_float)this->grafRTObjects->hdrRTImage->GetDesc().Size.x;
		cb.SrcTargetSize.y = (ur_float)this->grafRTObjects->hdrRTImage->GetDesc().Size.y;
		dynamicCBAlloc = grafRenderer->GetDynamicConstantBufferAllocation(sizeof(ConstantsCB));
		dynamicCB->Write((ur_byte*)&cb, sizeof(cb), 0, dynamicCBAlloc.Offset);

		descTable = this->grafObjects->tonemapPSDescTables[frameIdx].get();
		descTable->SetConstantBuffer(1, dynamicCB, dynamicCBAlloc.Offset, dynamicCBAlloc.Size);
		descTable->SetSampler(0, this->grafObjects->samplerPoint.get());
		descTable->SetSampler(1, this->grafObjects->samplerLinear.get());
		descTable->SetImage(0, this->grafRTObjects->hdrRTImage.get());
		descTable->SetImage(1, this->grafRTObjects->lumRTChainImages.back().get());
		descTable->SetImage(2, this->grafRTObjects->bloomImage[0].get());

		rtViewport.Width = (ur_float)resolveTarget->GetImage(0)->GetDesc().Size.x;
		rtViewport.Height = (ur_float)resolveTarget->GetImage(0)->GetDesc().Size.y;

		grafCmdList.SetViewport(rtViewport, true);
		grafCmdList.ImageMemoryBarrier(this->grafRTObjects->lumRTChainImages.back().get(), GrafImageState::Current, GrafImageState::ShaderRead);
		grafCmdList.ImageMemoryBarrier(this->grafRTObjects->bloomImage[0].get(), GrafImageState::Current, GrafImageState::ShaderRead);
		grafCmdList.BeginRenderPass(this->grafObjects->tonemapRenderPass.get(), resolveTarget);
		grafCmdList.BindPipeline(this->grafObjects->tonemapPipeline.get());
		grafCmdList.BindDescriptorTable(descTable, this->grafObjects->tonemapPipeline.get());
		grafCmdList.Draw(4, 1, 0, 0);
		grafCmdList.EndRenderPass();

		return Result(Success);
	#endif
	}

	void HDRRender::ShowImgui()
	{
		if (ImGui::CollapsingHeader("HDR Rendering"))
		{
			ImGui::DragFloat("LumKey", &this->params.LumKey, 0.01f, 0.01f, 1.0f);
			ImGui::InputFloat("LumWhite", &this->params.LumWhite);
			ImGui::DragFloat("Bloom", &this->params.BloomThreshold, 0.01f, 0.01f, 100.0f);
			const char* DebugListBoxItems = "None\0HDR\0Bloom\0LumFirst\0LumLast\0";
			ImGui::Combo("DebugRT", (int*)&this->debugRT, DebugListBoxItems);
		}
	}

} // end namespace UnlimRealms