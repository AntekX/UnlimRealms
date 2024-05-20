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

	inline ur_uint RenderRealm::GetCanvasWidth() const
	{
		return this->GetCanvas()->GetClientBound().Width();
	}

	inline ur_uint RenderRealm::GetCanvasHeight() const
	{
		return this->GetCanvas()->GetClientBound().Height();
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