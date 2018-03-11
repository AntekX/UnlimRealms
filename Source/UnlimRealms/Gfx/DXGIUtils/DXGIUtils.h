///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Gfx/GfxSystem.h"
#include <dxgi.h>

namespace UnlimRealms
{

	extern UR_DECL GfxAdapterDesc DXGIAdapterDescToGfx(const DXGI_ADAPTER_DESC1 &desc);

	extern UR_DECL DXGI_FORMAT GfxFormatToDXGI(GfxFormat fmt, GfxFormatView view);

	extern UR_DECL DXGI_FORMAT GfxBitsPerIndexToDXGIFormat(ur_uint bitsPerIndex);

} // end namespace UnlimRealms
