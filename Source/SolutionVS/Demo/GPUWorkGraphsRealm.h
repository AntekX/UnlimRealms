#pragma once

#include "RenderRealm/RenderRealm.h"
using namespace UnlimRealms;

#define GPUWORKGRAPH_SAMPLE 0

class GPUWorkGraphsRealm : public WinRenderRealm
{
public:

	GPUWorkGraphsRealm();

	virtual ~GPUWorkGraphsRealm();

	Result Initialize();

	virtual Result InitializeGraphicObjects();

	virtual Result DeinitializeGraphicObjects();

	virtual Result Update(const UpdateContext& updateContext);

	virtual Result Render(const RenderContext& renderContext);

	virtual Result DisplayImgui();

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

	struct GraphicsObjects
	{
		std::unique_ptr<GrafShaderLib> proceduralGraphShaderLib;
		std::unique_ptr<GrafWorkGraphPipeline> proceduralGraphPipeline;
		std::unique_ptr<GrafDescriptorTableLayout> proceduralGraphDescTableLayout;
		std::unique_ptr<GrafManagedDescriptorTable> proceduralGraphDescTable;
		std::unique_ptr<GrafBuffer> partitionDataBuffer;
	};

	Result ProceduralRender(const RenderContext& renderContext);
	Result ProceduralDisplayImgui();

#endif

	std::unique_ptr<GraphicsObjects> graphicsObjects;
};