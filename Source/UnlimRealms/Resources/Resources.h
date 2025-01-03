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
	// Utilities
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Result UR_DECL CreateVertexShaderFromFile(Realm &realm, std::unique_ptr<GfxVertexShader> &resOutput, const std::string &resName);

	Result UR_DECL CreatePixelShaderFromFile(Realm &realm, std::unique_ptr<GfxPixelShader> &resOutput, const std::string &resName);

	Result UR_DECL CreateTextureFromFile(Realm &realm, std::unique_ptr<GfxTexture> &resOutput, const std::string &resName);

} // end namespace UnlimRealms