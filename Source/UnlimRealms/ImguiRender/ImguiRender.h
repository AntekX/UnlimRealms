///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Realm/Realm.h"
#include "Gfx/GfxSystem.h"
#include "3rdParty/imgui/imgui.h"

namespace UnlimRealms
{

	// WIP time definition
#define NEW_GAPI 1

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

	private:

		void ReleaseGfxObjects();

		std::unique_ptr<GfxVertexShader> gfxVS;
		std::unique_ptr<GfxPixelShader> gfxPS;
		std::unique_ptr<GfxInputLayout> gfxInputLayout;
		std::unique_ptr<GfxTexture> gfxFontsTexture;
		std::unique_ptr<GfxBuffer> gfxCB;
		std::unique_ptr<GfxBuffer> gfxVB;
		std::unique_ptr<GfxBuffer> gfxIB;
#if (NEW_GAPI)
		std::unique_ptr<GfxSampler> gfxSampler;
		std::unique_ptr<GfxPipelineStateObject> gfxPipelineState;
		std::unique_ptr<GfxResourceBinding> gfxResourceBinding;
#else
		std::unique_ptr<GfxPipelineState> gfxPipelineState;
#endif
		ClockTime timePoint;
	};

} // end namespace UnlimRealms