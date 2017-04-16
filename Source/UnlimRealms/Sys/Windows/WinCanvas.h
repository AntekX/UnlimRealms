///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Sys/Canvas.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Windows system canvas
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL WinCanvas : public Canvas
	{
	public:

		enum class Style
		{
			BorderlessWindow,
			OverlappedWindow,
		};

		WinCanvas(Realm &realm,
			Style style = Style::BorderlessWindow,
			const wchar_t *title = ur_null);

		virtual ~WinCanvas();

		inline const RectI& GetClientBound() const;

		inline HINSTANCE GetHinstance() const;

		inline HWND GetHwnd() const;

	private:

		virtual Result OnInitialize(const RectI &bound);

		virtual Result OnSetBound(const RectI &bound);

		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

		std::wstring title;
		Style style;
		HINSTANCE hinst;
		HWND hwnd;
	};

} // end namespace UnlimRealms

#include "Sys/Windows/WinCanvas.inline.h"