///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Multiverse/Multiverse.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Multiverse
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Multiverse::Multiverse(Realm &realm) :
		RealmEntity(realm)
	{

	}

	Multiverse::~Multiverse()
	{

	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Space
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Space::Space(Multiverse &multiverse) :
		RealmEntity(multiverse.GetRealm()),
		multiverse(multiverse)
	{

	}

	Space::~Space()
	{

	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Space
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	SpaceObject::SpaceObject(Space &space) :
		RealmEntity(space.GetRealm()),
		space(space)
	{

	}

	SpaceObject::~SpaceObject()
	{

	}

} // end namespace UnlimRealms