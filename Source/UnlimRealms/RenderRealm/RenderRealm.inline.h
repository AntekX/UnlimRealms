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

	inline GrafRenderer* RenderRealm::GetGrafRenderer()
	{
		return this->grafRenderer;
	}

} // end namespace UnlimRealms