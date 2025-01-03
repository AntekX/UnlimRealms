#pragma once

#include "RenderRealm/RenderRealm.h"
#include "Camera/CameraControl.h"

using namespace UnlimRealms;

#define GPUWORKGRAPH_SAMPLE 0

class GPUWorkGraphsRealm : public WinRenderRealm
{
public:

	GPUWorkGraphsRealm();

	virtual ~GPUWorkGraphsRealm();

	virtual Result Initialize();

	virtual Result Update(const UpdateContext& updateContext);

	virtual Result Render(const RenderContext& renderContext);

	virtual Result DisplayImgui();

protected:

	virtual Result InitializeGraphicObjects();

	virtual Result DeinitializeGraphicObjects();

	virtual Result InitializeCanvasObjects();

	virtual Result SafeDeleteCanvasObjects(GrafCommandList* commandList);

private:

#if (GPUWORKGRAPH_SAMPLE)

	struct GraphicsObjects
	{
		std::unique_ptr<GrafShaderLib> workGraphShaderLib;
		std::unique_ptr<GrafWorkGraphPipeline> workGraphPipeline;
		std::unique_ptr<GrafDescriptorTableLayout> workGraphDescTableLayout;
		std::unique_ptr<GrafManagedDescriptorTable> workGraphDescTable;
		std::unique_ptr<GrafBuffer> workGraphDataBuffer;
		std::unique_ptr<GrafBuffer> workGraphReadbackBuffer;
		std::vector<ur_byte> workGraphReadbackData;
		std::unique_ptr<GrafFence> workGraphReadbackFence;
	};

	Result SampleRender(const RenderContext& renderContext);
	Result SampleDisplayImgui();

#else

	struct CanvasObjects
	{
		std::vector<std::unique_ptr<GrafImage>> rtImages;
		std::vector<std::unique_ptr<GrafRenderTarget>> renderTargets;
		std::vector<std::unique_ptr<GrafRenderTarget>> directRenderTargets;
		ur_bool resetHistory;
	};

	struct GraphicsObjects
	{
		std::unique_ptr<GrafShaderLib> proceduralGraphShaderLib;
		std::unique_ptr<GrafWorkGraphPipeline> proceduralGraphPipeline;
		std::unique_ptr<GrafDescriptorTableLayout> proceduralGraphDescTableLayout;
		std::unique_ptr<GrafManagedDescriptorTable> proceduralGraphDescTable;
		std::unique_ptr<GrafBuffer> partitionDataBuffer;
		std::unique_ptr<GrafBuffer> readbackBuffer;
		std::vector<ur_byte> readbackData;
		std::unique_ptr<GrafFence> readbackFence;
		ur_bool readbackPending;
		std::unique_ptr<GrafRenderPass> directRenderPass;
		std::unique_ptr<GrafRenderPass> gbufferRenderPass;
		std::unique_ptr<GrafShaderLib> proceduralRenderShaderLib;
		std::unique_ptr<GrafDescriptorTableLayout> proceduralRenderDescTableLayout;
		std::unique_ptr<GrafManagedDescriptorTable> proceduralRenderDescTable;
		std::unique_ptr<GrafPipeline> proceduralRenderPipeline;
		std::unique_ptr<CanvasObjects> canvas;
	};

	struct ProceduralObject
	{
		ur_float3 position;
		ur_float extent;
	};

	Result ProceduralUpdate(const UpdateContext& renderContext);
	Result ProceduralRender(const RenderContext& renderContext);
	Result ProceduralDisplayImgui();

	ProceduralObject proceduralObject;
	Camera camera;
	CameraControl cameraControl;
	ur_float3 refinementPoint;
	ur_bool freezeRefPoint;

#endif

	std::unique_ptr<GraphicsObjects> graphicsObjects;
};