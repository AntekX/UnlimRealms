///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Realm/Realm.h"
#include "Gfx/GfxSystem.h"
#include "GenericRender/GenericRender.h"

namespace UnlimRealms
{

#define NEW_GAPI 1

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// HDR rendering
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL HDRRender : public RealmEntity
	{
	public:

		struct UR_DECL Params
		{
			ur_float LumKey;
			ur_float LumWhite;
			ur_float BloomThreshold;

			static const Params Default;
		};

		HDRRender(Realm &realm);

		~HDRRender();

		Result SetParams(const Params &params);

		Result Init(ur_uint width, ur_uint height, bool depthStencilEnabled = true);

		Result BeginRender(GfxContext &gfxContext);

		Result EndRender(GfxContext &gfxContext);

		Result Resolve(GfxContext &gfxContext);

		void ShowImgui();

		inline GfxRenderTarget* GetHDRTarget() const { return (gfxObjects != ur_null ? this->gfxObjects->hdrRT.get() : ur_null); }

	protected:

		Result CreateGfxObjects();

		static GfxRenderState AverageLuminanceRenderState;

		struct alignas(16) ConstantsCB
		{
			ur_float2 SrcTargetSize;
			ur_float BlurDirection;
			Params params;
		};

		struct GfxObjects
		{
			std::unique_ptr<GfxRenderTarget> hdrRT;
			std::vector<std::unique_ptr<GfxRenderTarget>> lumRTChain;
			std::unique_ptr<GfxRenderTarget> bloomRT[2];
			std::unique_ptr<GfxBuffer> constantsCB;
			std::unique_ptr<GfxPixelShader> HDRTargetLuminancePS;
			std::unique_ptr<GfxPixelShader> bloomLuminancePS;
			std::unique_ptr<GfxPixelShader> averageLuminancePS;
			std::unique_ptr<GfxPixelShader> toneMappingPS;
			std::unique_ptr<GfxPixelShader> blurPS;
			std::unique_ptr<GenericRender::PipelineState> screenQuadStateHDRLuminance;
			std::unique_ptr<GenericRender::PipelineState> screenQuadStateAverageLuminance;
			std::unique_ptr<GenericRender::PipelineState> screenQuadStateBloom;
			std::unique_ptr<GenericRender::PipelineState> screenQuadStateBlur;
			std::unique_ptr<GenericRender::PipelineState> screenQuadStateTonemapping;
			std::unique_ptr<GenericRender::PipelineState> screenQuadStateDebug;

		};

		std::unique_ptr<GfxObjects> gfxObjects;
		Params params;
		
		enum DebugRT
		{
			DebugRT_None,
			DebugRT_HDR,
			DebugRT_Bloom,
			DebugRT_LumFirst,
			DebugRT_LumLast,
		};
		DebugRT debugRT;
	};

} // end namespace UnlimRealms