///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{

	inline void Canvas::SetPos(ur_int x, ur_int y)
	{
		this->bound.Move(x, y);
		this->SetBound(this->bound);
	}

	inline void Canvas::SetSize(ur_int width, ur_int height)
	{
		this->bound.Resize(width, height);
		this->SetBound(this->bound);
	}

	inline const RectI& Canvas::GetBound() const
	{
		return this->bound;
	}

	inline const RectI& Canvas::GetClientBound() const
	{
		return this->clientBound;
	}

} // end namespace UnlimRealms