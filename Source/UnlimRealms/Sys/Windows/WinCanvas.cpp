///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Sys/Log.h"
#include "Sys/Windows/WinCanvas.h"

namespace UnlimRealms
{

	static const wchar_t WndClassName[] = L"UnlimRealms_WinCanvas_WndClass";

	WinCanvas::WinCanvas(Realm &realm, Style style, const wchar_t *title) :
		Canvas(realm)
	{
		this->title = (title ? title : L"UnlimRealms");
		this->style = style;
		this->hinst = NULL;
		this->hwnd = NULL;
	}

	WinCanvas::~WinCanvas()
	{
		if (this->hwnd != NULL)
		{
			DestroyWindow(this->hwnd);
		}
		if (this->hinst != NULL)
		{
			UnregisterClassW(WndClassName, this->hinst);
		}
	}

	Result WinCanvas::OnInitialize(const RectI &bound)
	{
		// get active process handle

		this->hinst = GetModuleHandle(NULL);

		// register window class
		
		WNDCLASSEX wcex;
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.lpszClassName = WndClassName;
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WinCanvas::WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = this->hinst;
		wcex.hIcon = ur_null;
		wcex.hCursor = LoadCursor(ur_null, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = ur_null;
		wcex.hIconSm = ur_null;

		if (!RegisterClassExW(&wcex))
			return ResultError(Failure, "WinCanvas: failed to regidter window class");
		
		// create window

		DWORD dwStyle = 0;
		switch (this->style)
		{
			case Style::BorderlessWindow: dwStyle = WS_OVERLAPPED | WS_POPUP; break;
			case Style::OverlappedWindow:
			case Style::OverlappedWindowMaximized:
				dwStyle = WS_OVERLAPPEDWINDOW;
				break;
		}

		this->hwnd = CreateWindowW(WndClassName, this->title.c_str(), dwStyle,
			bound.Min.x, bound.Min.y, bound.Width(), bound.Height(),
			ur_null, ur_null, this->hinst, this);
		if (!this->hwnd)
			ResultError(Failure, "WinCanvas: failed to create window");

		ur_int showMode = (Style::OverlappedWindowMaximized == this->style ? SW_SHOWMAXIMIZED : SW_NORMAL);
		ShowWindow(this->hwnd, showMode);
		UpdateWindow(this->hwnd);

		RECT wndRect;
		GetClientRect(this->hwnd, &wndRect);
		this->clientBound = RectI(ur_int(wndRect.left), ur_int(wndRect.top), ur_int(wndRect.right), ur_int(wndRect.bottom));

		return ResultNote(Success, "WinCanvas: initialized");
	}

	Result WinCanvas::OnSetBound(const RectI &bound)
	{
		if (ur_null == this->hwnd)
			ResultWarning(NotInitialized, "WinCanvas: trying to move uninitialized windows");
		
		BOOL res = MoveWindow(this->hwnd, bound.Min.x, bound.Min.y, bound.Width(), bound.Height(), FALSE);

		RECT wndRect;
		GetClientRect(this->hwnd, &wndRect);
		this->clientBound = RectI(ur_int(wndRect.left), ur_int(wndRect.top), ur_int(wndRect.right), ur_int(wndRect.bottom));

		return (TRUE == res ? Result(Success) : ResultError(Failure, "WinCanvas: failed to move window"));
	}

	LRESULT CALLBACK WinCanvas::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		WinCanvas *canvas = ur_null;

		if (WM_NCCREATE == message)
		{
			LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			canvas = static_cast<WinCanvas*>(lpcs->lpCreateParams);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(canvas));
		}
		else
		{
			canvas = reinterpret_cast<WinCanvas*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		}
		
		switch (message)
		{
		case WM_SIZE:
		case WM_MOVE:
		{
			RECT wndRect;
			if (canvas != ur_null &&
				canvas->hwnd != ur_null &&
				GetWindowRect(hWnd, &wndRect))
			{
				canvas->SetBound( RectI(wndRect.left, wndRect.top, wndRect.right, wndRect.bottom) );
			}
		}
		break;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			if (canvas != ur_null)
			{
				// redraw
			}
			EndPaint(hWnd, &ps);
		}
		break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}

		return 0;
	}

} // end namespace UnlimRealms