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
	// HSR rendering
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL HDRRender : public RealmEntity
	{
	public:

		HDRRender(Realm &realm);

		~HDRRender();

		Result Init(ur_uint width, ur_uint height, bool depthStencilEnabled = true);

		Result BeginRender(GfxContext &gfxContext);

		Result EndRender(GfxContext &gfxContext);

		Result Resolve(GfxContext &gfxContext);

	protected:

		Result CreateGfxObjects();

		struct GfxObjects
		{
			std::unique_ptr<GfxRenderTarget> hdrRT;
			std::unique_ptr<GfxRenderTarget> luminanceRT;
			std::unique_ptr<GfxPixelShader> computeLuminancePS;
			std::unique_ptr<GfxPixelShader> tonemappingPS;
		};

		std::unique_ptr<GfxObjects> gfxObjects;
	};

} // end namespace UnlimRealms