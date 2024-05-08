///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// D3D12 SDK symbols override
// must be included in application's main.cpp
#include "Graf/DX12/GrafSystemDX12.h"
#if (UR_GRAF_DX12_AGILITY_SDK_VERSION)
extern "C" { __declspec(dllexport) extern const unsigned int D3D12SDKVersion = UR_GRAF_DX12_AGILITY_SDK_VERSION; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }
#endif