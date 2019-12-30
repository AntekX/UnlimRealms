///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Realm/Realm.h"
#include "Gfx/GfxSystem.h"
#include "Graf/GrafRenderer.h"
#include "3rdParty/imgui/imgui.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Imgui Renderer
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL ImguiRender : public RealmEntity
	{
	public:

		ImguiRender(Realm &realm);

		virtual ~ImguiRender();

		Result Init();

		Result NewFrame();

		Result Render(GfxContext &gfxContext);

		Result Render(GrafCommandList& grafCmdList);

	private:

		void InitKeyMapping();

		void ReleaseGfxObjects();

		#if defined(UR_GRAF)
		GrafRenderer* grafRenderer;
		ur_float2 crntImguiDisplaySize;
		std::unique_ptr<GrafShader> grafVS;
		std::unique_ptr<GrafShader> grafPS;
		std::unique_ptr<GrafBuffer> grafVB;
		std::unique_ptr<GrafBuffer> grafIB;
		std::unique_ptr<GrafBuffer> grafCB;
		std::unique_ptr<GrafDescriptorTableLayout> grafBindingLayout;
		std::unique_ptr<GrafPipeline> grafPipeline;
		std::vector<std::unique_ptr<GrafDescriptorTable>> grafBindingTables;
		std::unique_ptr<GrafSampler> grafSampler;
		std::unique_ptr<GrafImage> grafFontImage;
		#else
		std::unique_ptr<GfxVertexShader> gfxVS;
		std::unique_ptr<GfxPixelShader> gfxPS;
		std::unique_ptr<GfxInputLayout> gfxInputLayout;
		std::unique_ptr<GfxTexture> gfxFontsTexture;
		std::unique_ptr<GfxBuffer> gfxCB;
		std::unique_ptr<GfxBuffer> gfxVB;
		std::unique_ptr<GfxBuffer> gfxIB;
		std::unique_ptr<GfxPipelineState> gfxPipelineState;
		#endif
		ClockTime timePoint;
	};

} // end namespace UnlimRealms