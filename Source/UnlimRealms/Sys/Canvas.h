///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Realm/Realm.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base system canvas
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
	class UR_DECL Canvas : public RealmEntity
	{
	public:

		enum class Style
		{
			BorderlessWindow,
			OverlappedWindow,
			OverlappedWindowMaximized,
		};

		Canvas(Realm &realm, Style style = Style::BorderlessWindow, const wchar_t* title = ur_null);

		virtual ~Canvas();

		Result Initialize(const RectI &bound);

		Result SetBound(const RectI &bound);

		inline const Style& GetStyle() const;

		inline const std::wstring& GetTitle() const;

		inline void SetPos(ur_int x, ur_int y);

		inline void SetSize(ur_int width, ur_int height);

		inline const RectI& GetBound() const;

		inline const RectI& GetClientBound() const;

	protected:
		
		virtual Result OnInitialize(const RectI &bound);

		virtual Result OnSetBound(const RectI &bound);

	protected:

		Style style;
		std::wstring title;
		RectI clientBound;

	private:

		RectI bound;
	};

} // end namespace UnlimRealms

#include "Sys/Canvas.inline.h"