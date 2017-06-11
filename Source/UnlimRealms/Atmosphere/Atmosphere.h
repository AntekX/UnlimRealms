///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Realm/Realm.h"
#include "Gfx/GfxSystem.h"

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
			static const Desc Default;
		};

		Atmosphere(Realm &realm);

		virtual ~Atmosphere();

		Result Init(const Desc &desc);

		Result Render(GfxContext &gfxContext, const ur_float4x4 &viewProj, const ur_float3 &cameraPos);

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
		} gfxObjects;

		struct alignas(16) CommonCB
		{
			ur_float4x4 ViewProj;
			ur_float4 CameraPos;
			Desc Params;
		};

		struct Vertex
		{
			ur_float3 pos;
		};

		typedef ur_uint16 Index;

		Desc desc;
	};

} // end namespace UnlimRealms