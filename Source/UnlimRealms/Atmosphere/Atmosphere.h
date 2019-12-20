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

		void ShowImgui();

		inline const Desc& GetDesc() const { return this->desc; }

	protected:

		Result CreateGfxObjects();

		Result CreateMesh();

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