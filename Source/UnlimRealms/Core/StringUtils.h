///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common.h"
#include "Core/BaseTypes.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// String utils
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	void UR_DECL StringToWstring(const std::string& src, std::wstring& dst);

	void UR_DECL WstringToString(const std::wstring& src, std::string& dst);

} // end namespace UnlimRealms