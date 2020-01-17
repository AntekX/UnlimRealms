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

			res = GrafUtils::CreateShaderFromFile(*grafDevice, "HDRTargetLuminance_ps.spv", GrafShaderType::Pixel, grafObjects->calculateLuminancePS);
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to create shader for HDRTargetLuminance_ps");

			res = GrafUtils::CreateShaderFromFile(*grafDevice, "ToneMapping_ps.spv", GrafShaderType::Pixel, grafObjects->toneMappingPS);
			if (Failed(res))
				return ResultError(Failure, "HDRRender::Init: failed to create shader for ToneMapping_ps");

			this->grafObjects.reset(grafObjects.release());
		}

		// (re)create RTs and images

		if (this->grafRTObjects != ur_null)
		{
			this->grafRenderer->SafeDelete(this->grafRTObjects.release());
		}
		
		std::unique_ptr<GrafRTObjects> grafRTObjects(new GrafRTObjects(*this->grafRenderer->GetGrafSystem()));
		
		// render pass

		res = grafSystem->CreateRenderPass(grafRTObjects->hdrRenderPass);
		if (Succeeded(res))
		{
			GrafRenderPassImageDesc hdrRenderPassDesc[] = {
				{ // color
					GrafFormat::R16G16B16A16_SFLOAT,
					GrafImageState::Undefined, GrafImageState::ColorWrite,
					GrafRenderPassDataOp::Clear, GrafRenderPassDataOp::Store,
					GrafRenderPassDataOp::DontCare, GrafRenderPassDataOp::DontCare
				},
				{ // depth
					GrafFormat::D24_UNORM_S8_UINT,
					GrafImageState::Undefined, GrafImageState::DepthStencilWrite,
					GrafRenderPassDataOp::Clear, GrafRenderPassDataOp::Store,
					GrafRenderPassDataOp::DontCare, GrafRenderPassDataOp::DontCare
				}
			};
			res = grafRTObjects->hdrRenderPass->Initialize(grafDevice, { hdrRenderPassDesc, (depthStnecilRTImage ? 2u : 1u) });
		}
		if (Failed(res))
			return ResultError(Failure, "HDRRender::Init: failed to initialize render pass");

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
			res = grafRTObjects->hdrRT->Initialize(grafDevice, { grafRTObjects->hdrRenderPass.get(), hdrRTImages, (depthStnecilRTImage ? 2u : 1u) } );
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
				res = avgLumRT->Initialize(grafDevice, { grafRTObjects->hdrRenderPass.get(), lumRTImages, 1 });
			}
			if (Failed(res))
				break;
			
			lumRTImageDesc.Size.x = std::max(ur_uint(1), lumRTImageDesc.Size.x >> 1);
			lumRTImageDesc.Size.y = std::max(ur_uint(1), lumRTImageDesc.Size.y >> 1);
		}
		if (Failed(res))
			return ResultError(Failure, "HDRRender::Init: failed to initialize luminance image(s)");

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
		grafCmdList.BeginRenderPass(this->grafRTObjects->hdrRenderPass.get(), this->grafRTObjects->hdrRT.get(), hdrRTClearValues);

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

	Result HDRRender::Resolve(GrafCommandList &grafCmdList, GrafImage* colorTargetImage)
	{
	#if !defined(UR_GRAF)
		return Result(NotImplemented);
	#else

		// TODO: do tonemapping

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