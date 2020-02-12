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
	// Atmosphere
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL Atmosphere : public RealmEntity
	{
	public:

		struct UR_DECL Desc
		{
			ur_float InnerRadius;
			ur_float OuterRadius;
			ur_float ScaleDepth;
			ur_float G;
			ur_float Km;
			ur_float Kr;
			ur_float D;
			static const Desc Default;
			static const Desc Invisible;
		};

		struct UR_DECL LightShaftsDesc
		{
			float Density;
			float DensityMax;
			float Weight;
			float Decay;
			float Exposure;
			static const LightShaftsDesc Default;
		};

		Atmosphere(Realm &realm);

		virtual ~Atmosphere();

		Result Init(const Desc &desc);

		Result Render(GfxContext &gfxContext, const ur_float4x4 &viewProj, const ur_float3 &cameraPos);

		Result RenderPostEffects(GfxContext &gfxContext, GfxRenderTarget &renderTarget,
			const ur_float4x4 &viewProj, const ur_float3 &cameraPos);

		Result Init(const Desc& desc, GrafRenderPass* grafRenderPass);

		Result Render(GrafCommandList &grafCmdList, const ur_float4x4 &viewProj, const ur_float3 &cameraPos);

		Result RenderPostEffects(GrafCommandList &grafCmdList, GrafRenderTarget &renderTarget,
			const ur_float4x4 &viewProj, const ur_float3 &cameraPos);

		void ShowImgui();

		inline const Desc& GetDesc() const { return this->desc; }

	protected:

		Result CreateMesh();

		#if defined(UR_GRAF)

		class GrafObjects : public GrafEntity
		{
		public:
			std::unique_ptr<GrafShader> VS;
			std::unique_ptr<GrafShader> PS;
			std::unique_ptr<GrafShader> PSDbg;
			std::unique_ptr<GrafShader> fullScreenQuadVS;
			std::unique_ptr<GrafShader> fullScreenQuadPS;
			std::unique_ptr<GrafShader> lightShaftsPS;
			std::unique_ptr<GrafDescriptorTableLayout> shaderDescriptorLayout;
			std::unique_ptr<GrafDescriptorTableLayout> fullscreenQuadDescriptorLayout;
			std::unique_ptr<GrafDescriptorTableLayout> lightShaftsApplyDescriptorLayout;
			std::vector<std::unique_ptr<GrafDescriptorTable>> shaderDescriptorTable;
			std::vector<std::unique_ptr<GrafDescriptorTable>> lightShaftsMaskDescriptorTable;
			std::vector<std::unique_ptr<GrafDescriptorTable>> lightShaftsApplyDescriptorTable;
			std::unique_ptr<GrafRenderPass> lightShaftsMaskRenderPass;
			std::unique_ptr<GrafRenderPass> lightShaftsApplyRenderPass;
			std::unique_ptr<GrafPipeline> pipelineSolid;
			std::unique_ptr<GrafPipeline> pipelineDebug;
			std::unique_ptr<GrafPipeline> pipelineLightShaftsMask;
			std::unique_ptr<GrafPipeline> pipelineLightShaftsApply;
			std::unique_ptr<GrafBuffer> VB;
			std::unique_ptr<GrafBuffer> IB;
			std::unique_ptr<GrafImage> lightShaftsRTImage;
			std::unique_ptr<GrafRenderTarget> lightShaftsRT;
			std::unique_ptr<GrafSampler> samplerPoint;
			std::unique_ptr<GrafSampler> samplerLinear;
			GrafObjects(GrafSystem& grafSystem);
			~GrafObjects();
		};
		std::unique_ptr<GrafObjects> grafObjects;
		GrafRenderer* grafRenderer;

		Result CreateGrafObjects(GrafRenderPass* grafRenderPass);

		#else

		struct GfxObjects
		{
			std::unique_ptr<GfxVertexShader> VS;
			std::unique_ptr<GfxPixelShader> PS;
			std::unique_ptr<GfxPixelShader> PSDbg;
			std::unique_ptr<GfxInputLayout> inputLayout;
			std::unique_ptr<GfxBuffer> CB;
			std::unique_ptr<GfxPipelineState> pipelineState;
			std::unique_ptr<GfxPipelineState> wireframeState;
			std::unique_ptr<GfxBuffer> VB;
			std::unique_ptr<GfxBuffer> IB;
			std::unique_ptr<GfxPixelShader> lightShaftsPS;
			std::unique_ptr<GfxRenderTarget> lightShaftsRT;
			std::unique_ptr<GfxBuffer> lightShaftsCB;
			std::unique_ptr<GenericRender::State> screenQuadStateOcclusionMask;
			std::unique_ptr<GenericRender::State> screenQuadStateBlendLightShafts;
			std::unique_ptr<GfxSampler> pointSampler;
		} gfxObjects;
		
		Result CreateGfxObjects();

		#endif

		struct alignas(16) CommonCB
		{
			ur_float4x4 ViewProj;
			ur_float4 CameraPos;
			Desc Params;
		};

		struct alignas(16) LightShaftsCB : public LightShaftsDesc
		{
		};

		struct Vertex
		{
			ur_float3 pos;
		};

		typedef ur_uint16 Index;

		Desc desc;
		LightShaftsDesc lightShafts;
	};

} // end namespace UnlimRealms