///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{
	inline Realm& RenderRealm::GetRealm()
	{
		return *this;
	}

	inline void RenderRealm::SetState(State state)
	{
		this->state = state;
	}

	inline RenderRealm::State RenderRealm::GetState() const
	{
		return this->state;
	}

	inline ur_float RenderRealm::GetCanvasResolutionScale() const
	{
		return this->canvasResolutionScale;
	}

	inline ur_uint RenderRealm::GetCanvasWidth() const
	{
		return ur_uint(this->GetCanvas()->GetClientBound().Width() * this->canvasResolutionScale);
	}

	inline ur_uint RenderRealm::GetCanvasHeight() const
	{
		return ur_uint(this->GetCanvas()->GetClientBound().Height() * this->canvasResolutionScale);
	}

	inline GrafRenderer* RenderRealm::GetGrafRenderer()
	{
		return this->grafRenderer;
	}

	inline ImguiRender* RenderRealm::GetImguiRenderer()
	{
		return this->imguiRender;
	}

} // end namespace UnlimRealms