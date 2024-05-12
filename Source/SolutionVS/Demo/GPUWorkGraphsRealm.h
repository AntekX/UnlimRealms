#pragma once

#include "RenderRealm/RenderRealm.h"
using namespace UnlimRealms;

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

private:

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
	std::unique_ptr<GraphicsObjects> graphicsObjects;
};