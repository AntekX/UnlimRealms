///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{

	inline const RectI& WinCanvas::GetClientBound() const
	{
		RECT wndRect;
		GetClientRect(this->hwnd, &wndRect);
		return RectI(ur_int(wndRect.left), ur_int(wndRect.top), ur_int(wndRect.right), ur_int(wndRect.bottom));
	}

	inline HINSTANCE WinCanvas::GetHinstance() const
	{
		return this->hinst;
	}

	inline HWND WinCanvas::GetHwnd() const
	{
		return this->hwnd;
	}

} // end namespace UnlimRealms