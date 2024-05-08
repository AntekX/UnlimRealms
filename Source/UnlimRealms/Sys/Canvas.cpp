///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Sys/Log.h"
#include "Sys/Canvas.h"

namespace UnlimRealms
{

	Canvas::Canvas(Realm &realm, Style style, const wchar_t* title) :
		RealmEntity(realm),
		style(style),
		title(title)
	{
		LogNote("Canvas: created");
	}

	Canvas::~Canvas()
	{
		LogNote("Canvas: destroyed");
	}

	Result Canvas::Initialize(const RectI &bound)
	{
		if (bound.IsInsideOut())
			return ResultError(InvalidArgs, "Canvas: failed to initialize because of invalid bounding rect");

		this->bound = bound;

		return OnInitialize(bound);
	}

	Result Canvas::SetBound(const RectI &bound)
	{
		if (bound.IsInsideOut())
			return ResultWarning(InvalidArgs, "Canvas: can not set inside out bounding rect");

		this->bound = bound;
		this->clientBound = bound;

		return this->OnSetBound(bound);
	}

	Result Canvas::OnInitialize(const RectI &bound)
	{
		return Result(Success);
	}

	Result Canvas::OnSetBound(const RectI &bound)
	{
		return Result(Success);
	}

} // end namespace UnlimRealms