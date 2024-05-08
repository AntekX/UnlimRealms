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
	};
	std::unique_ptr<GraphicsObjects> graphicsObjects;
};