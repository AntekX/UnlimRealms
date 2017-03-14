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

		Atmosphere(Realm &realm);

		virtual ~Atmosphere();

		Result Init(ur_float radius);

		Result Render(GfxContext &gfxContext, const ur_float4x4 &viewProj, const ur_float3 &cameraPos);

	protected:

		Result CreateGfxObjects();

		Result CreateMesh(ur_float radius);

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

		struct CommonCB
		{
			ur_float4x4 viewProj;
			ur_float4 cameraPos;
		};

		struct Vertex
		{
			ur_float3 pos;
		};

		typedef ur_uint16 Index;
	};

} // end namespace UnlimRealms