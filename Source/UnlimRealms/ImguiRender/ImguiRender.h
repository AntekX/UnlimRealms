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
		std::unique_ptr<GfxPipelineState> gfxPipelineState;
		ClockTime timePoint;
	};

} // end namespace UnlimRealms