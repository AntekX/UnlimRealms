///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef _UNLIMREALMS_DLL
#define UR_DECL __declspec(dllexport)
#else
#define UR_DECL __declspec(dllimport)
#endif

#define GDIPLUS

#ifdef _WINDOWS

// Windows

#ifdef _WIN_x86
#define UR_PLATFORM_x86
#elif _WIN_x64
#define UR_PLATFORM_x64
#endif

#include <SDKDDKVer.h>
#include <windows.h>
#include <process.h>

#ifdef GDIPLUS

// GDI

#include <Unknwn.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#endif

#undef min
#undef max

#endif

// STL

#include <array>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <list>
#include <algorithm>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <functional>

// STL shortcuts

typedef std::chrono::steady_clock Clock;
typedef std::chrono::time_point<std::chrono::steady_clock> ClockTime;
#define ClockDeltaAs std::chrono::duration_cast

// STL exported types

#pragma warning(disable : 4251) // class needs to have dll-interface

//template class UR_DECL std::allocator<char>;
//template class UR_DECL std::allocator<wchar_t>;
//template class UR_DECL std::basic_string<char, std::char_traits<char>, std::allocator<char> >;
//template class UR_DECL std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >;

// DirectXToolKit

//#define DXTK
//#include "..\\3rdParty\\DirectXTex\\DirectXTex.h"
//#ifdef _DEBUG
//#ifdef WIN64
//#pragma comment(lib, "..\\3rdParty\\DirectXTex\\x64\\Debug\\DirectXTex.lib")
//#else
//#pragma comment(lib, "..\\3rdParty\\DirectXTex\\Debug\\DirectXTex.lib")
//#endif
//#else
//#ifdef WIN64
//#pragma comment(lib, "..\\3rdParty\\DirectXTex\\x64\\Release\\DirectXTex.lib")
//#else
//#pragma comment(lib, "..\\3rdParty\\DirectXTex\\Release\\DirectXTex.lib")
//#endif
//#endif