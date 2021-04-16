#ifndef HYBRID_RENDERING_COMMON_HLSLI
#define HYBRID_RENDERING_COMMON_HLSLI

#include "ShaderLib/CommonTypes.hlsli"
#include "Atmosphere/AtmosphereCommon.hlsli"

struct SceneConstants
{
	CFLOAT4x4	(View);
	CFLOAT4x4	(Proj);
	CFLOAT4x4	(ViewProj);
	CFLOAT4x4	(ViewProjInv);
	CFLOAT4x4	(ViewPrev);
	CFLOAT4x4	(ProjPrev);
	CFLOAT4x4	(ViewProjPrev);
	CFLOAT4x4	(ViewProjInvPrev);
	CFLOAT4		(CameraPos);
	CFLOAT4		(CameraDir);
	CFLOAT4		(TargetSize); // w, h, 1/w, 1/h
	CFLOAT4		(LightBufferSize); // w, h, 1/w, 1/h
	CFLOAT4		(PrecomputedSkySize);
	CFLOAT4		(DebugVec0);
	CFLOAT4		(DebugVec1);
	CFLOAT2		(LightBufferDownscale); // d, 1/d
	CFLOAT		(DirectLightFactor);
	CFLOAT		(IndirectLightFactor);
	CUINT		(FrameNumber);
	CUINT		(ShadowSamplesPerLight);
	CUINT		(ShadowAccumulationFrames);
	CUINT		(IndirectSamplesPerFrame);
	CUINT		(IndirectAccumulationFrames);
	CUINT		(PerFrameJitter);
	CUINT		(OverrideMaterial);
	CUINT		(__pad1);
	LightingDesc Lighting;
	AtmosphereDesc Atmosphere;
	MeshMaterialDesc Material;
};

#endif