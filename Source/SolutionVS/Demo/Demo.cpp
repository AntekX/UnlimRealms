// Demo.cpp : Defines the entry point for the application.

#include "stdafx.h"
#include "Demo.h"
#include "UnlimRealmsAppConfig.h"

#include "VoxelPlanetApp.h"
#include "D3D12SandboxApp.h"
#include "VulkanSandboxApp.h"
#include "RayTracingSandboxApp.h"
#include "HybridRenderingApp.h"
#include "GPUWorkGraphsRealm.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
#if (1)
	//VoxelPlanetApp demoApp;
	//D3D12SandboxApp demoApp;
	//VulkanSandboxApp demoApp;
	//RayTracingSandboxApp demoApp;
	HybridRenderingApp demoApp;
	return demoApp.Run();
#else
	std::unique_ptr<RenderRealm> demoRealm(new GPUWorkGraphsRealm());
	Result res = demoRealm->Initialize();
	if (Succeeded(res))
	{
		res = demoRealm->Run();
		demoRealm->Deinitialize();
	}
	return res;
#endif
}