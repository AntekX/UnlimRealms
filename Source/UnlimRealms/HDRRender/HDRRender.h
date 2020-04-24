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
			ur_float BloomIntensity;

			static const Params Default;
		};

		HDRRender(Realm &realm);

		~HDRRender();

		Result SetParams(const Params &params);
		
		Result Init(ur_uint width, ur_uint height, bool depthStencilEnabled = true);

		Result BeginRender(GfxContext &gfxContext);

		Result EndRender(GfxContext &gfxContext);

		Result Resolve(GfxContext &gfxContext);

		Result Init(ur_uint width, ur_uint height, GrafImage* depthStnecilRTImage);

		Result BeginRender(GrafCommandList &grafCmdList);

		Result EndRender(GrafCommandList &grafCmdList);

		Result Resolve(GrafCommandList &grafCmdList, GrafRenderTarget* resolveTarget);

		void ShowImgui();

		inline GfxRenderTarget* GetHDRTarget() const
		{
		#if defined(UR_GRAF)
			return ur_null;
		#else
			return (gfxObjects != ur_null ? this->gfxObjects->hdrRT.get() : ur_null);
		#endif
		}

		inline GrafRenderPass* GetRenderPass() const
		{
		#if defined(UR_GRAF)
			return (this->grafObjects != ur_null ? this->grafObjects->hdrRenderPass.get() : ur_null);
		#else
			return ur_null;
		#endif
		}

		inline GrafRenderTarget* GetHDRRenderTarget() const
		{
		#if defined(UR_GRAF)
			return (this->grafRTObjects != ur_null ? this->grafRTObjects->hdrRT.get() : ur_null);
		#else
			return ur_null;
		#endif
		}
	
	protected:

		static GfxRenderState AverageLuminanceRenderState;

		struct alignas(16) ConstantsCB
		{
			ur_float2 SrcTargetSize;
			ur_float BlurDirection;
			Params params;
		};

		#if defined(UR_GRAF)

		class GrafObjects : public GrafEntity
		{
		public:
			std::unique_ptr<GrafShader> fullScreenQuadVS;
			std::unique_ptr<GrafShader> luminancePS;
			std::unique_ptr<GrafShader> luminanceAvgPS;
			std::unique_ptr<GrafShader> bloomPS;
			std::unique_ptr<GrafShader> blurPS;
			std::unique_ptr<GrafShader> tonemapPS;
			std::unique_ptr<GrafDescriptorTableLayout> luminancePSDescLayout;
			std::unique_ptr<GrafDescriptorTableLayout> tonemapPSDescLayout;
			std::vector<std::unique_ptr<GrafDescriptorTable>> tonemapPSDescTables;
			std::vector<std::unique_ptr<GrafDescriptorTable>> bloomPSDescTables;
			std::unique_ptr<GrafRenderPass> hdrRenderPass;
			std::unique_ptr<GrafRenderPass> lumRenderPass;
			std::unique_ptr<GrafRenderPass> bloomRenderPass;
			std::unique_ptr<GrafRenderPass> tonemapRenderPass;
			std::unique_ptr<GrafPipeline> lumPipeline;
			std::unique_ptr<GrafPipeline> avgPipeline;
			std::unique_ptr<GrafPipeline> bloomPipeline;
			std::unique_ptr<GrafPipeline> blurPipeline;
			std::unique_ptr<GrafPipeline> tonemapPipeline;
			std::unique_ptr<GrafSampler> samplerLinear;
			std::unique_ptr<GrafSampler> samplerPoint;
			GrafObjects(GrafSystem& grafSystem);
			~GrafObjects();
		};
		class GrafRTObjects : public GrafEntity
		{
		public:
			std::unique_ptr<GrafRenderTarget> hdrRT;
			std::unique_ptr<GrafImage> hdrRTImage;
			std::vector<std::unique_ptr<GrafRenderTarget>> lumRTChain;
			std::vector<std::unique_ptr<GrafImage>> lumRTChainImages;
			std::vector<std::unique_ptr<GrafDescriptorTable>> lumRTChainTables;
			std::unique_ptr<GrafImage> bloomImage[2];
			std::unique_ptr<GrafRenderTarget> bloomRT[2];
			GrafRTObjects(GrafSystem& grafSystem);
			~GrafRTObjects();
		};
		std::unique_ptr<GrafObjects> grafObjects;
		std::unique_ptr<GrafRTObjects> grafRTObjects;
		GrafRenderer* grafRenderer;
		
		#else

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
			std::unique_ptr<GenericRender::State> screenQuadStateHDRLuminance;
			std::unique_ptr<GenericRender::State> screenQuadStateAverageLuminance;
			std::unique_ptr<GenericRender::State> screenQuadStateBloom;
			std::unique_ptr<GenericRender::State> screenQuadStateBlur;
			std::unique_ptr<GenericRender::State> screenQuadStateTonemapping;
			std::unique_ptr<GenericRender::State> screenQuadStateDebug;
		};
		std::unique_ptr<GfxObjects> gfxObjects;

		Result CreateGfxObjects();

		#endif

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