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
	// Base terrain sub system
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Terrain::SubSystem::SubSystem(Terrain &terrain) :
		terrain(terrain)
	{

	}

	Terrain::SubSystem::~SubSystem()
	{
	
	}

	Result Terrain::SubSystem::Create(InstanceHandle& instanceHandle, const InstanceDesc &instanceDesc)
	{
		return NotImplemented;
	}

	Result Terrain::SubSystem::AddInstance(InstanceHandle& instanceHandle, std::unique_ptr<Instance> &instance)
	{
		instanceHandle = InvalidHandle;
		if (ur_null == instance.get())
			return InvalidArgs;

		static ur_size InstanceHandleIdx = 0;
		instanceHandle = InstanceHandleIdx++;
		this->instances.insert(std::pair<InstanceHandle, std::unique_ptr<Instance>>(instanceHandle, std::move(instance)));
		
		return Success;
	}

	Result Terrain::SubSystem::RemoveInstance(const InstanceHandle& instanceHandle)
	{
		auto record = this->instances.find(instanceHandle);
		if (this->instances.end() == record)
			return NotFound;

		this->instances.erase(record);

		return Success;
	}


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

	Result Terrain::ProceduralData::Create(InstanceHandle& instanceHandle, const ProceduralData::InstanceDesc &desc)
	{
		return NotImplemented;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Base presentation
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Result Terrain::Presentation::Render(GfxContext &gfxContext, const ur_float4x4 &viewProj, const ur_float3 &cameraPos, const Atmosphere *atmosphere)
	{
		return NotImplemented;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// SimpleGrid
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Terrain::SimpleGrid::SimpleGrid(Terrain &terrain) :
		Terrain::Presentation(terrain)
	{
	}

	Terrain::SimpleGrid::~SimpleGrid()
	{
	}

	Result Terrain::SimpleGrid::Create(InstanceHandle& instanceHandle, const SimpleGrid::InstanceDesc &desc)
	{
		// TODO
		return NotImplemented;
	}

	Result Terrain::SimpleGrid::Render(GfxContext &gfxContext, const ur_float4x4 &viewProj, const ur_float3 &cameraPos, const Atmosphere *atmosphere)
	{
		// TODO
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

		InstanceHandle hinstance;
		this->Create<ProceduralData, SimpleGrid>(hinstance, ProceduralData::InstanceDesc(), SimpleGrid::InstanceDesc());
		
		// nothing to do
		return Success;
	}

	Result Terrain::Update()
	{
		return Success;
	}

	Result Terrain::Render(GfxContext &gfxContext, const ur_float4x4 &viewProj, const ur_float3 &cameraPos, const Atmosphere *atmosphere)
	{

		return Success;
	}

} // end namespace UnlimRealms