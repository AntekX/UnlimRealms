// Demo.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Demo.h"
#include "VoxelPlanetApp.h"
#include "D3D12SandboxApp.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	VoxelPlanetApp demoApp;
	//D3D12SandboxApp demoApp;
	
	return demoApp.Run();
}