///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Terrain/Terrain.h"
#include "Sys/Storage.h"
#include "Sys/Log.h"
#include "Sys/Canvas.h"
#include "Sys/Input.h"
#include "Resources/Resources.h"
#include "ImguiRender/ImguiRender.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Procedural data generator
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	Terrain::ProceduralData::ProceduralData(Terrain &terrain) : 
		Terrain::Data(terrain)
	{
	}

	Terrain::ProceduralData::~ProceduralData()
	{
	}

	Result Terrain::ProceduralData::Create(Handle& instanceHandle, const ProceduralData::InstanceDesc &desc)
	{
		return NotImplemented;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base presentation
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Terrain::SimpleGrid::SimpleGrid(Terrain &terrain) :
		Terrain::Presentation(terrain)
	{
	}

	Terrain::SimpleGrid::~SimpleGrid()
	{
	}

	Result Terrain::SimpleGrid::Create(Handle& instanceHandle, const SimpleGrid::InstanceDesc &desc)
	{
		return NotImplemented;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Terrain
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Terrain::Terrain(Realm &realm) : 
		RealmEntity(realm)
	{

	}

	Terrain::~Terrain()
	{

	}

	Result Terrain::Init()
	{
		// test
		this->RegisterSubSystem<ProceduralData>();
		this->RegisterSubSystem<SimpleGrid>();

		Handle hinstance;		
		this->Create<ProceduralData, SimpleGrid>(hinstance, ProceduralData::InstanceDesc(), SimpleGrid::InstanceDesc());
		
		// nothing to do
		return Success;
	}

} // end namespace UnlimRealms