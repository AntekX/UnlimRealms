///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Renderaing application base helper class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "UnlimRealms.h"
#include "Core/BaseTypes.h"
#include "Core/ResultTypes.h"
#include "Sys/Canvas.h"
#include "Graf/GrafRenderer.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL RenderRealm : public Realm
	{
	public:

		enum class State
		{
			Initialize,
			Run,
			Finish
		};

		enum class GrafSystemType
		{
			Undefined,
			DX12,
			Vulkan
		};

		struct UR_DECL InitParams
		{
			const wchar_t* Title;
			Canvas::Style CanvasStyle;
			GrafSystemType GrafSystemType;
			GrafRenderer::InitParams RendererParams;
			static const InitParams Default;
		};

		struct UR_DECL UpdateContext
		{
			ur_int64 CurrentRunTime; // ms
			ur_int64 ElapsedTime; // ms
		};

		struct UR_DECL RenderContext
		{
		};

		RenderRealm();
		
		virtual ~RenderRealm();

		Result Initialize(const InitParams& initParams);

		virtual Result Initialize();

		virtual Result Deinitialize();

		virtual Result Run();

		virtual Result Update(const UpdateContext& updateContext);

		virtual Result Render(const RenderContext& renderContext);

		inline Realm& GetRealm();

		inline void SetState(State state);

		inline State GetState() const;

		inline GrafRenderer* GetGrafRenderer();

	protected:

		Result InitializeGrafRenderer(GrafSystemType grafSystemType, const GrafRenderer::InitParams& rendererParams);

		Result DeintializeGrafRenderer();

		virtual Result CreateSystemCanvas(std::unique_ptr<Canvas>& canvas, Canvas::Style style, const wchar_t* title);

		virtual Result CreateSystemInput(std::unique_ptr<Input>& input);

		virtual Result InitializeGraphicObjects(); // called after GrafRenderer initialization
		
		virtual Result DeinitializeGraphicObjects(); // called before GrafRenderer deinitialization

		State state;

	private:

		GrafRenderer* grafRenderer; // component shortcut
	};

#if defined(_WINDOWS)

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class UR_DECL WinRenderRealm : public RenderRealm
	{
	public:

		WinRenderRealm();

		virtual ~WinRenderRealm();

		virtual Result Run();

	protected:

		virtual Result CreateSystemCanvas(std::unique_ptr<Canvas>& canvas, Canvas::Style style, const wchar_t* title);

		virtual Result CreateSystemInput(std::unique_ptr<Input>& input);
	};

#endif // _WINDOWS

} // end namespace UnlimRealms

#include "RenderRealm/RenderRealm.inline.h"