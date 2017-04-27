///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{

	inline HINSTANCE WinCanvas::GetHinstance() const
	{
		return this->hinst;
	}

	inline HWND WinCanvas::GetHwnd() const
	{
		return this->hwnd;
	}

} // end namespace UnlimRealms