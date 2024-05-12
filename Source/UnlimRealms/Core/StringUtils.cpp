///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Core/StringUtils.h"

namespace UnlimRealms
{

	void StringToWstring(const std::string& src, std::wstring& dst)
	{
		#pragma warning(push)
		#pragma warning(disable: 4996) // std::wstring_convert is depricated
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> strConverter;
		dst = std::wstring(strConverter.from_bytes(src));
		#pragma warning(pop)
	}

	void WstringToString(const std::wstring& src, std::string& dst)
	{
		#pragma warning(push)
		#pragma warning(disable: 4996) // std::wstring_convert is depricated
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> strConverter;
		dst = std::string(strConverter.to_bytes(src));
		#pragma warning(pop)
	}

} // end namespace UnlimRealms