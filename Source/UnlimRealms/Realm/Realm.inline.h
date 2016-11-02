///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace UnlimRealms
{

	inline Storage& Realm::GetStorage()
	{
		return *this->storage.get();
	}

	inline Log& Realm::GetLog()
	{
		return *this->log.get();
	}

	inline Input* Realm::GetInput() const
	{
		return this->input.get();
	}

	inline Canvas* Realm::GetCanvas() const
	{
		return this->canvas.get();
	}

	inline GfxSystem* Realm::GetGfxSystem() const
	{
		return this->gfxSystem.get();
	}

	inline Realm& RealmEntity::GetRealm()
	{
		return this->realm;
	}

} // end namespace UnlimRealms